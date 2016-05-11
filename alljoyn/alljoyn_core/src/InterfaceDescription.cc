/*
 * @file
 *
 * This file implements the InterfaceDescription class
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/XmlElement.h>
#include <map>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Status.h>

#include "SignatureUtils.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {


static size_t GetAnnotationsWithValues(
    const std::map<qcc::String, qcc::String>& annotations,
    qcc::String* names,
    qcc::String* values,
    size_t size)
{
    size_t count = annotations.size();

    if (names && values) {
        typedef std::map<qcc::String, qcc::String> AnnotationsMap;
        count = std::min(count, size);
        AnnotationsMap::const_iterator mit = annotations.begin();
        for (size_t i = 0; i < count && mit != annotations.end(); ++i, ++mit) {
            names[i] = mit->first;
            values[i] = mit->second;
        }
    }

    return count;
}

class InterfaceDescription::AnnotationsMap : public std::map<qcc::String, qcc::String> {
};

class InterfaceDescription::ArgumentDescriptions : public std::map<qcc::String, qcc::String> {
};

class InterfaceDescription::ArgumentAnnotations : public std::map<std::pair<qcc::String, qcc::String>, qcc::String> {
};

qcc::String InterfaceDescription::NextArg(const char*& signature, qcc::String& argNames, bool inOut,
                                          size_t indent, Member const& member, bool withDescriptions,
                                          const char* langTag, Translator* translator) const
{
    qcc::String argName;
    qcc::String in(indent, ' ');
    qcc::String arg = in + "<arg";
    qcc::String argType;
    const char* start = signature;
    SignatureUtils::ParseCompleteType(signature);
    size_t len = signature - start;
    argType.append(start, len);

    if (!argNames.empty()) {
        size_t pos = argNames.find_first_of(',');
        argName = argNames.substr(0, pos);
        arg += " name=\"" + argName + "\"";
        if (pos == qcc::String::npos) {
            argNames.clear();
        } else {
            argNames.erase(0, pos + 1);
        }
    }
    arg += " type=\"" + argType + "\" direction=\"";
    arg += inOut ? "in\"" : "out\"";

    qcc::String childNodesXml;
    const char* myDesc = NULL;
    ArgumentDescriptions::const_iterator search = member.argumentDescriptions->find(argName);
    if (member.argumentDescriptions->end() != search) {
        myDesc = search->second.c_str();
    }
    if (myDesc || member.argumentAnnotations->size() > 0) {
        if (myDesc) {
            if (withDescriptions) {
                qcc::String value;
                member.GetArgAnnotation(argName.c_str(), qcc::String("org.alljoyn.Bus.DocString.") + langTag, value);
                AppendDescriptionXml(childNodesXml, langTag, myDesc, translator, value.c_str(), in);
            }
            if (langTag == NULL) {
                AppendDescriptionToArgAnnotations(*member.argumentAnnotations, argName.c_str(), myDesc, translator);
            }
        }

        /* Add annotations that belongs to this argument */
        ArgumentAnnotations::const_iterator ait = member.argumentAnnotations->begin();
        for (; ait != member.argumentAnnotations->end(); ++ait) {
            if (ait->first.first == argName) {
                childNodesXml += in + "  <annotation name=\"" + ait->first.second.c_str() + "\" value=\"" + ait->second + "\"/>\n";
            }
        }
    }
    if (!childNodesXml.empty()) {
        arg += ">\n" + childNodesXml + in + "</arg>\n";
    } else {
        arg += "/>\n";
    }

    return arg;
}

void InterfaceDescription::AppendDescriptionXml(qcc::String& xml, const char* language,
                                                const char* localDescription, Translator* translator,
                                                const char* languageDescription,
                                                qcc::String const& indent) const
{
    qcc::String buffer;
    const char* d;
    if ((languageDescription != nullptr) && (languageDescription[0] != '\0') && ((translator == nullptr) || (localDescription == nullptr))) {
        // Language description is from DocString annotation.
        // Use it only when the local description cannot be translated
        d = languageDescription;
    } else {
        d = Translate(language, localDescription, buffer, translator);
        if (!d || d[0] == '\0') {
            return;
        }
    }
    xml += indent + "  <description>" + XmlElement::EscapeXml(d) + "</description>\n";
}

InterfaceDescription::Member::Member(const InterfaceDescription* iface,
                                     AllJoynMessageType type,
                                     const char* name,
                                     const char* signature,
                                     const char* returnSignature,
                                     const char* argNames,
                                     uint8_t annotation,
                                     const char* accessPerms) :
    iface(iface),
    memberType(type),
    name(name),
    signature(signature ? signature : ""),
    returnSignature(returnSignature ? returnSignature : ""),
    argNames(argNames ? argNames : ""),
    annotations(new AnnotationsMap()),
    accessPerms(accessPerms ? accessPerms : ""),
    description(),
    argumentDescriptions(new ArgumentDescriptions()),
    isSessioncastSignal(false),
    isSessionlessSignal(false),
    isUnicastSignal(false),
    isGlobalBroadcastSignal(false),
    argumentAnnotations(new ArgumentAnnotations())
{
    if (annotation & MEMBER_ANNOTATE_DEPRECATED) {
        (*annotations)[org::freedesktop::DBus::AnnotateDeprecated] = "true";
    }

    if (annotation & MEMBER_ANNOTATE_NO_REPLY) {
        (*annotations)[org::freedesktop::DBus::AnnotateNoReply] = "true";
    }

    if (annotation & MEMBER_ANNOTATE_SESSIONCAST) {
        isSessioncastSignal = true;
    }

    if (annotation & MEMBER_ANNOTATE_SESSIONLESS) {
        isSessionlessSignal = true;
    }

    if (annotation & MEMBER_ANNOTATE_UNICAST) {
        isUnicastSignal = true;
    }

    if (annotation & MEMBER_ANNOTATE_GLOBAL_BROADCAST) {
        isGlobalBroadcastSignal = true;
    }
}

InterfaceDescription::Member::Member(const Member& other)
    : iface(other.iface),
    memberType(other.memberType),
    name(other.name),
    signature(other.signature),
    returnSignature(other.returnSignature),
    argNames(other.argNames),
    annotations(new AnnotationsMap(*(other.annotations))),
    accessPerms(other.accessPerms),
    description(other.description),
    argumentDescriptions(new ArgumentDescriptions(*(other.argumentDescriptions))),
    isSessioncastSignal(other.isSessioncastSignal),
    isSessionlessSignal(other.isSessionlessSignal),
    isUnicastSignal(other.isUnicastSignal),
    isGlobalBroadcastSignal(other.isGlobalBroadcastSignal),
    argumentAnnotations(new ArgumentAnnotations(*(other.argumentAnnotations)))
{
}

InterfaceDescription::Member& InterfaceDescription::Member::operator=(const Member& other)
{
    if (this != &other) {
        iface = other.iface;
        memberType = other.memberType;
        name = other.name;
        signature = other.signature;
        returnSignature = other.returnSignature;
        argNames = other.argNames;
        delete annotations;
        annotations = new AnnotationsMap(*(other.annotations));
        accessPerms = other.accessPerms;
        description = other.description;
        delete argumentDescriptions;
        argumentDescriptions = new ArgumentDescriptions(*(other.argumentDescriptions));
        delete argumentAnnotations;
        argumentAnnotations = new ArgumentAnnotations(*(other.argumentAnnotations));
        isSessioncastSignal = other.isSessioncastSignal;
        isSessionlessSignal = other.isSessionlessSignal;
        isUnicastSignal = other.isUnicastSignal;
        isGlobalBroadcastSignal = other.isGlobalBroadcastSignal;
    }
    return *this;
}

InterfaceDescription::Member::~Member()
{
    delete annotations;
    delete argumentDescriptions;
    delete argumentAnnotations;
}

size_t InterfaceDescription::Member::GetAnnotations(qcc::String* names, qcc::String* values, size_t size) const
{
    return GetAnnotationsWithValues(*annotations, names, values, size);
}

size_t InterfaceDescription::Member::GetArgAnnotations(const char* argName, qcc::String* names, qcc::String* values, size_t size) const
{
    size_t count = argumentAnnotations->size();

    if ((names != NULL) && (values != NULL)) {
        ArgumentAnnotations::const_iterator ait = argumentAnnotations->begin();
        count = std::min(count, size);
        for (size_t i = 0; (i < count) && (ait != argumentAnnotations->end()); ++ait) {
            if (ait->first.first == argName) {
                names[i] = ait->first.second;
                values[i] = ait->second;
                ++i;
            }
        }
    }

    return count;
}

bool InterfaceDescription::Member::operator==(const Member& o) const {
    return ((memberType == o.memberType) && (name == o.name) && (signature == o.signature)
            && (returnSignature == o.returnSignature) && (*annotations == *(o.annotations))
            && (description == o.description) && (*argumentDescriptions == *(o.argumentDescriptions))
            && (*argumentAnnotations == *(o.argumentAnnotations))
            && (isSessioncastSignal == o.isSessioncastSignal)
            && (isSessionlessSignal == o.isSessionlessSignal)
            && (isUnicastSignal == o.isUnicastSignal)
            && (isGlobalBroadcastSignal == o.isGlobalBroadcastSignal));
}


InterfaceDescription::Property::Property(const char* name, const char* signature, uint8_t access) :
    name(name),
    signature(signature ? signature : ""),
    access(access),
    annotations(new AnnotationsMap()),
    description(),
    cacheable(false)
{
}


InterfaceDescription::Property::Property(const char* name, const char* signature, uint8_t access, uint8_t annotation) :
    name(name),
    signature(signature ? signature : ""),
    access(access),
    annotations(new AnnotationsMap()),
    description(),
    cacheable(false)
{
    if (annotation & PROP_ANNOTATE_EMIT_CHANGED_SIGNAL) {
        (*annotations)[org::freedesktop::DBus::AnnotateEmitsChanged] = "true";
        cacheable = true;
    }
    if (annotation & PROP_ANNOTATE_EMIT_CHANGED_SIGNAL_CONST) {
        (*annotations)[org::freedesktop::DBus::AnnotateEmitsChanged] = "const";
        cacheable = true;
    }
    if (annotation & PROP_ANNOTATE_EMIT_CHANGED_SIGNAL_INVALIDATES) {
        (*annotations)[org::freedesktop::DBus::AnnotateEmitsChanged] = "invalidates";
        cacheable = true;
    }
}

InterfaceDescription::Property::Property(const Property& other)
    : name(other.name),
    signature(other.signature),
    access(other.access),
    annotations(new AnnotationsMap(*(other.annotations))),
    description(other.description),
    cacheable(other.cacheable)
{

}

InterfaceDescription::Property& InterfaceDescription::Property::operator=(const Property& other)
{
    if (this != &other) {
        name = other.name;
        signature = other.signature;
        access = other.access;
        delete annotations;
        annotations = new AnnotationsMap(*(other.annotations));
        description = other.description;
    }
    return *this;
}

InterfaceDescription::Property::~Property()
{
    delete annotations;
}

size_t InterfaceDescription::Property::GetAnnotations(qcc::String* names, qcc::String* values, size_t size) const
{
    return GetAnnotationsWithValues(*annotations, names, values, size);
}

/** Equality */
bool InterfaceDescription::Property::operator==(const Property& o) const {
    return ((name == o.name) && (signature == o.signature) && (access == o.access) && (*annotations == *(o.annotations))
            && description == o.description);
}

struct InterfaceDescription::Definitions {
    typedef std::map<std::string, Member> MemberMap;
    typedef std::map<std::string, Property> PropertyMap;

    MemberMap members;              /**< Interface members */
    PropertyMap properties;         /**< Interface properties */
    AnnotationsMap annotations;     /**< Interface Annotations */
    qcc::String languageTag;
    qcc::String description;
    StringTableTranslator stringTableTranslator;
    Translator* translator;
    bool hasDescription;

    Definitions() :
        translator(&stringTableTranslator), hasDescription(false)
    { }

    Definitions(const Definitions& other) :
        members(other.members), properties(other.properties), annotations(other.annotations),
        languageTag(other.languageTag), description(other.description), hasDescription(other.hasDescription)
    {
        bool usingDefaultTranslator = (other.translator == &other.stringTableTranslator);
        if (usingDefaultTranslator) {
            translator = &stringTableTranslator;
            *translator = *other.translator;
        } else {
            translator = other.translator;
        }
    }

    Definitions& operator=(const Definitions& other)
    {
        if (this != &other) {
            members = other.members;
            properties = other.properties;
            annotations = other.annotations;
            languageTag = other.languageTag;
            description = other.description;
            bool usingDefaultTranslator = (other.translator == &other.stringTableTranslator);
            if (usingDefaultTranslator) {
                translator = &stringTableTranslator;
                *translator = *other.translator;
            } else {
                translator = other.translator;
            }
            hasDescription = other.hasDescription;
        }
        return *this;
    }
};

bool InterfaceDescription::Member::GetAnnotation(const qcc::String& annotationName, qcc::String& value) const
{
    AnnotationsMap::const_iterator it = annotations->find(annotationName);
    return (it != annotations->end() ? value = it->second, true : false);
}

bool InterfaceDescription::Member::GetArgAnnotation(const char* argName, const qcc::String& annotationName, qcc::String& value) const
{
    pair<qcc::String, qcc::String> item(qcc::String(argName), annotationName);
    ArgumentAnnotations::const_iterator ait = argumentAnnotations->find(item);
    return (ait != argumentAnnotations->end() ? value = ait->second, true : false);
}

bool InterfaceDescription::Property::GetAnnotation(const qcc::String& annotationName, qcc::String& value) const
{
    AnnotationsMap::const_iterator it = annotations->find(annotationName);
    return (it != annotations->end() ? value = it->second, true : false);
}

InterfaceDescription::InterfaceDescription(const char* name, InterfaceSecurityPolicy secPolicy) :
    defs(new Definitions),
    name(name),
    isActivated(false),
    secPolicy(secPolicy)
{
    if (secPolicy != AJ_IFC_SECURITY_INHERIT) {
        /*
         * We don't allow a secure annotation on the standard DBus Interfaces
         */
        if ((strcmp(name, org::freedesktop::DBus::Introspectable::InterfaceName) != 0) &&
            (strcmp(name, org::freedesktop::DBus::Peer::InterfaceName)           != 0) &&
            (strcmp(name, org::freedesktop::DBus::Properties::InterfaceName)     != 0)) {
            defs->annotations[org::alljoyn::Bus::Secure] = (secPolicy == AJ_IFC_SECURITY_REQUIRED) ? "true" : "off";
        }
    }
}

InterfaceDescription::~InterfaceDescription()
{
    delete defs;
}

InterfaceDescription::InterfaceDescription(const InterfaceDescription& other) :
    defs(new Definitions(*other.defs)),
    name(other.name),
    isActivated(false),
    secPolicy(other.secPolicy)
{
    /* Update the iface pointer in each member */
    Definitions::MemberMap::iterator mit = defs->members.begin();
    while (mit != defs->members.end()) {
        mit++->second.iface = this;
    }
}

InterfaceDescription& InterfaceDescription::operator=(const InterfaceDescription& other)
{
    if (this != &other) {
        *defs = *other.defs;
        name = other.name;
        isActivated = false;
        secPolicy = other.secPolicy;
        /* Update the iface pointer in each definitions member */
        Definitions::MemberMap::iterator mit = defs->members.begin();
        while (mit != defs->members.end()) {
            mit++->second.iface = this;
        }
    }
    return *this;
}

qcc::String InterfaceDescription::Introspect(size_t indent, const char* languageTag, Translator* translator) const
{
    qcc::String in(indent, ' ');
    const qcc::String close = "\">\n";

    Translator* myTranslator = defs->translator;
    if (!defs->translator ||
        ((defs->translator == &defs->stringTableTranslator) &&
         defs->stringTableTranslator.IsEmpty())) {
        myTranslator = translator;
    }

    bool withDescriptions = languageTag && defs->hasDescription;
    bool unifiedFormat = (languageTag == NULL);

    qcc::String xml = in + "<interface name=\"";
    xml += name + close;

    if (withDescriptions) {
        qcc::String value;
        GetAnnotation(qcc::String("org.alljoyn.Bus.DocString.") + languageTag, value);
        AppendDescriptionXml(xml, languageTag, defs->description.c_str(), myTranslator, value.c_str(), in);
    }
    if (unifiedFormat && defs->hasDescription) {
        AppendDescriptionToAnnotations(defs->annotations, defs->description.c_str(), myTranslator);
    }

    /*
     * Iterate over interface defs->members
     */
    Definitions::MemberMap::const_iterator mit = defs->members.begin();
    while (mit != defs->members.end()) {
        const Member& member = mit->second;
        qcc::String argNames = member.argNames;

        bool isMethod = (member.memberType == MESSAGE_METHOD_CALL);
        qcc::String mtype = isMethod ? "method" : "signal";
        xml += in + "  <" + mtype + " name=\"" + member.name;
        if (withDescriptions && !isMethod) {
            if (member.isSessioncastSignal) {
                xml += "\" sessioncast=\"" + qcc::String("true");
            }

            // For backwards compatibility reasons, always output a sessionless
            // attribute.
            xml += "\" sessionless=\"" + (member.isSessionlessSignal ? qcc::String("true") : qcc::String("false"));

            if (member.isUnicastSignal) {
                xml += "\" unicast=\"" + qcc::String("true");
            }
            if (member.isGlobalBroadcastSignal) {
                xml += "\" globalbroadcast=\"" + qcc::String("true");
            }
        }
        xml     += close;

        if (unifiedFormat && !isMethod) {
            if (member.isSessioncastSignal) {
                (*member.annotations)["org.alljoyn.Bus.Signal.Sessioncast"] = "true";
            }
            if (member.isSessionlessSignal) {
                (*member.annotations)["org.alljoyn.Bus.Signal.Sessionless"] = "true";
            }
            if (member.isUnicastSignal) {
                (*member.annotations)["org.alljoyn.Bus.Signal.Unicast"] = "true";
            }
            if (member.isGlobalBroadcastSignal) {
                (*member.annotations)["org.alljoyn.Bus.Signal.GlobalBroadcast"] = "true";
            }
        }

        if (withDescriptions) {
            qcc::String value;
            member.GetAnnotation(qcc::String("org.alljoyn.Bus.DocString.") + languageTag, value);
            AppendDescriptionXml(xml, languageTag, member.description.c_str(), myTranslator, value.c_str(), in + "  ");
        }
        if (unifiedFormat) {
            AppendDescriptionToAnnotations(*member.annotations, member.description.c_str(), myTranslator);
        }

        /* Iterate over IN arguments */
        for (const char* sig = member.signature.c_str(); *sig;) {
            // always treat signals as direction=out
            xml += NextArg(sig, argNames, member.memberType != MESSAGE_SIGNAL,
                           indent + 4, member, withDescriptions, languageTag, myTranslator);
        }
        /* Iterate over OUT arguments */
        for (const char* sig = member.returnSignature.c_str(); *sig;) {
            xml += NextArg(sig, argNames, false, indent + 4, member, withDescriptions, languageTag, myTranslator);
        }
        /*
         * Add annotations
         */

        AnnotationsMap::const_iterator ait = member.annotations->begin();
        for (; ait != member.annotations->end(); ++ait) {
            xml += in + "    <annotation name=\"" + ait->first.c_str() + "\" value=\"" + ait->second + "\"/>\n";
        }

        xml += in + "  </" + mtype + ">\n";
        ++mit;
    }
    /*
     * Iterate over interface properties
     */
    Definitions::PropertyMap::const_iterator pit = defs->properties.begin();
    while (pit != defs->properties.end()) {
        const Property& property = pit->second;
        xml += in + "  <property name=\"" + property.name + "\" type=\"" + property.signature + "\"";
        if (property.access == PROP_ACCESS_READ) {
            xml += " access=\"read\"";
        } else if (property.access == PROP_ACCESS_WRITE) {
            xml += " access=\"write\"";
        } else {
            xml += " access=\"readwrite\"";
        }

        // Does this property have a description? Only if
        // (a) the property has some description text, (b) that text is not a
        // lookup key (empty language tag) with no Translator to produce a description string,
        // and (c) the translator has a description in the requested language.
        bool propWithDescription = !property.description.empty() &&
                                   !(defs->languageTag.empty() && !myTranslator);
        if (propWithDescription) {
            qcc::String buffer;
            const char* d = Translate(languageTag, property.description.c_str(), buffer, myTranslator);
            if (!d || d[0] == '\0') {
                propWithDescription = false;
            }
        }

        // Does this property element have any sub-elements?
        if (property.annotations->size() || propWithDescription) {
            xml += ">\n";

            if (withDescriptions) {
                qcc::String value;
                property.GetAnnotation(qcc::String("org.alljoyn.Bus.DocString.") + languageTag, value);
                AppendDescriptionXml(xml, languageTag, property.description.c_str(), myTranslator, value.c_str(), in + "  ");
            }
            if (unifiedFormat) {
                AppendDescriptionToAnnotations(*property.annotations, property.description.c_str(), myTranslator);
            }

            if (property.annotations->size()) {
                // add annotations
                AnnotationsMap::const_iterator ait = property.annotations->begin();
                for (; ait != property.annotations->end(); ++ait) {
                    xml += in + "    <annotation name=\"" + ait->first.c_str() + "\" value=\"" + ait->second + "\"/>\n";
                }
            }
            xml += in + "  </property>\n";
        } else {
            xml += "/>\n";
        }

        ++pit;
    }

    // add interface annotations
    AnnotationsMap::const_iterator ait = defs->annotations.begin();
    for (; ait != defs->annotations.end(); ++ait) {
        xml += in + "  <annotation name=\"" + ait->first.c_str() + "\" value=\"" + ait->second + "\"/>\n";
    }

    xml += in + "</interface>\n";
    return xml;
}

QStatus InterfaceDescription::AddMember(AllJoynMessageType type,
                                        const char* memberName,
                                        const char* inSig,
                                        const char* outSig,
                                        const char* argNames,
                                        uint8_t annotation,
                                        const char* accessPerms)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    std::string key = qcc::String(memberName);
    Member member(this, type, memberName, inSig, outSig, argNames, annotation, accessPerms);
    pair<std::string, Member> item(key, member);
    pair<Definitions::MemberMap::iterator, bool> ret = defs->members.insert(item);
    return ret.second ? ER_OK : ER_BUS_MEMBER_ALREADY_EXISTS;
}

QStatus InterfaceDescription::AddMemberAnnotation(const char* member, const qcc::String& annotationName, const qcc::String& value)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::MemberMap::iterator it = defs->members.find(std::string(member));
    if (it == defs->members.end()) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }

    Member& m = it->second;
    std::pair<AnnotationsMap::iterator, bool> ret = m.annotations->insert(AnnotationsMap::value_type(annotationName, value));
    return (ret.second || (ret.first->first == annotationName && ret.first->second == value)) ? ER_OK : ER_BUS_ANNOTATION_ALREADY_EXISTS;
}

bool InterfaceDescription::GetMemberAnnotation(const char* member, const qcc::String& annotationName, qcc::String& value) const
{
    Definitions::MemberMap::const_iterator mit = defs->members.find(std::string(member));
    if (mit == defs->members.end()) {
        return false;
    }

    const Member& m = mit->second;
    AnnotationsMap::const_iterator ait = m.annotations->find(annotationName);
    return (ait != m.annotations->end() ? value = ait->second, true : false);
}


QStatus InterfaceDescription::AddProperty(const char* propertyName, const char* signature, uint8_t access)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    std::string key = qcc::String(propertyName);
    Property prop(propertyName, signature, access);
    pair<std::string, Property> item(key, prop);
    pair<Definitions::PropertyMap::iterator, bool> ret = defs->properties.insert(item);
    return ret.second ? ER_OK : ER_BUS_PROPERTY_ALREADY_EXISTS;
}

QStatus InterfaceDescription::AddPropertyAnnotation(const qcc::String& p_name, const qcc::String& annotationName, const qcc::String& value)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::PropertyMap::iterator pit = defs->properties.find(std::string(p_name));
    if (pit == defs->properties.end()) {
        return ER_BUS_NO_SUCH_PROPERTY;
    }

    Property& property = pit->second;
    std::pair<AnnotationsMap::iterator, bool> ret = property.annotations->insert(AnnotationsMap::value_type(annotationName, value));
    QStatus status = (ret.second || (ret.first->first == annotationName && ret.first->second == value)) ? ER_OK : ER_BUS_ANNOTATION_ALREADY_EXISTS;
    if (status == ER_OK) {
        if ((annotationName == org::freedesktop::DBus::AnnotateEmitsChanged) && (value != "false")) {
            property.cacheable = true;
        } else if ((annotationName == "org.alljoyn.Bus.Type.Min") ||
                   (annotationName == "org.alljoyn.Bus.Type.Max") ||
                   (annotationName == "org.alljoyn.Bus.Type.Units") ||
                   (annotationName == "org.alljoyn.Bus.Type.Default") ||
                   (annotationName == "org.alljoyn.Bus.Type.Reference") ||
                   (annotationName == "org.alljoyn.Bus.Type.DisplayHint")) {
            property.cacheable = true;
        }
    }
    return status;
}

bool InterfaceDescription::GetPropertyAnnotation(const qcc::String& p_name, const qcc::String& annotationName, qcc::String& value) const
{
    Definitions::PropertyMap::const_iterator pit = defs->properties.find(std::string(p_name));
    if (pit == defs->properties.end()) {
        return false;
    }

    const Property& property = pit->second;
    AnnotationsMap::const_iterator ait = property.annotations->find(annotationName);
    return (ait != property.annotations->end() ? value = ait->second, true : false);
}

QStatus InterfaceDescription::AddAnnotation(const qcc::String& annotationName, const qcc::String& value)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    std::pair<AnnotationsMap::iterator, bool> ret = defs->annotations.insert(std::make_pair(annotationName, value));
    return (ret.second || (ret.first->first == annotationName && ret.first->second == value)) ? ER_OK : ER_BUS_ANNOTATION_ALREADY_EXISTS;
}

bool InterfaceDescription::GetAnnotation(const qcc::String& annotationName, qcc::String& value) const
{
    AnnotationsMap::const_iterator it = defs->annotations.find(annotationName);
    return (it != defs->annotations.end() ? value = it->second, true : false);
}

size_t InterfaceDescription::GetAnnotations(qcc::String* names, qcc::String* values, size_t size) const
{
    return GetAnnotationsWithValues(defs->annotations, names, values, size);
}

bool InterfaceDescription::operator==(const InterfaceDescription& other) const
{
    if (name != other.name) {
        return false;
    }

    if (defs->members != other.defs->members ||
        defs->properties != other.defs->properties ||
        defs->annotations != other.defs->annotations) {
        return false;
    }

    return true;
}

bool InterfaceDescription::HasCacheableProperties() const
{
    Definitions::PropertyMap::const_iterator pit = defs->properties.begin();
    for (; pit != defs->properties.end(); ++pit) {
        if (pit->second.cacheable) {
            return true;
        }
    }
    return false;
}

size_t InterfaceDescription::GetProperties(const Property** props, size_t numProps) const
{
    size_t count = defs->properties.size();
    if (props) {
        count = min(count, numProps);
        Definitions::PropertyMap::const_iterator pit = defs->properties.begin();
        for (size_t i = 0; i < count && pit != defs->properties.end(); ++i, ++pit) {
            props[i] = &(pit->second);
        }
    }
    return count;
}

const InterfaceDescription::Property* InterfaceDescription::GetProperty(const char* propertyName) const
{
    Definitions::PropertyMap::const_iterator pit = defs->properties.find(std::string(propertyName));
    return (pit == defs->properties.end()) ? NULL : &(pit->second);
}

size_t InterfaceDescription::GetMembers(const Member** members, size_t numMembers) const
{
    size_t count = defs->members.size();
    if (members) {
        count = min(count, numMembers);
        Definitions::MemberMap::const_iterator mit = defs->members.begin();
        for (size_t i = 0; i < count && mit != defs->members.end(); ++i, ++mit) {
            members[i] = &(mit->second);
        }
    }
    return count;
}

const InterfaceDescription::Member* InterfaceDescription::GetMember(const char* memberName) const
{
    Definitions::MemberMap::const_iterator mit = defs->members.find(std::string(memberName));
    return (mit == defs->members.end()) ? NULL : &(mit->second);
}

bool InterfaceDescription::HasMember(const char* memberName, const char* inSig, const char* outSig)
{
    const Member* member = GetMember(memberName);
    if (member == NULL) {
        return false;
    } else if ((inSig == NULL) && (outSig == NULL)) {
        return true;
    } else {
        bool found = true;
        if (inSig) {
            found = found && (member->signature.compare(inSig) == 0);
        }
        if (outSig && (member->memberType == MESSAGE_METHOD_CALL)) {
            found = found && (member->returnSignature.compare(outSig) == 0);
        }
        return found;
    }
}

void InterfaceDescription::SetDescriptionLanguage(const char* language)
{
    defs->languageTag = language;
}

const char* InterfaceDescription::GetDescriptionLanguage() const
{
    return defs->languageTag.c_str();
}

void InterfaceDescription::SetDescription(const char* desc)
{
    defs->description = desc;
    defs->hasDescription = true;
}

QStatus InterfaceDescription::SetMemberDescription(const char* member, const char* desc)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::MemberMap::iterator it = defs->members.find(std::string(member));
    if (it == defs->members.end()) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }

    Member& m = it->second;
    m.description.assign(desc);

    defs->hasDescription = true;

    return ER_OK;
}

QStatus InterfaceDescription::SetMemberDescription(const char* member, const char* desc, bool isSessionlessSignal)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::MemberMap::iterator it = defs->members.find(std::string(member));
    if (it == defs->members.end()) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }

    Member& m = it->second;

    if (isSessionlessSignal && !m.isSessionlessSignal) {
        if (m.isSessioncastSignal || m.isUnicastSignal || m.isGlobalBroadcastSignal) {
            // The member was already set explicitly to not be sessionless,
            // so the caller must have a bug.
            QCC_LogError(ER_INVALID_SIGNAL_EMISSION_TYPE, ("Unexpected: SetMemberDescription tried to set isSessionlessSignal on a non-sessionless signal"));
            return ER_INVALID_SIGNAL_EMISSION_TYPE;
        } else {
            // Nothing was set before so set the signal type to sessionless.
            m.isSessionlessSignal = true;
        }
    }

    m.description.assign(desc);
    defs->hasDescription = true;

    return ER_OK;
}


QStatus InterfaceDescription::SetArgDescription(const char* member, const char* arg, const char* desc)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::MemberMap::iterator it = defs->members.find(std::string(member));
    if (it == defs->members.end()) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }

    Member& m = it->second;
    m.argumentDescriptions->insert(ArgumentDescriptions::value_type(arg, desc));
    defs->hasDescription = true;

    return ER_OK;
}

QStatus InterfaceDescription::SetPropertyDescription(const char* propName, const char* desc)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::PropertyMap::iterator pit = defs->properties.find(std::string(propName));
    if (pit == defs->properties.end()) {
        return ER_BUS_NO_SUCH_PROPERTY;
    }

    Property& property = pit->second;
    property.description.assign(desc);
    defs->hasDescription = true;

    return ER_OK;
}

bool InterfaceDescription::HasDescription() const
{
    return defs->hasDescription;
}

QStatus InterfaceDescription::AddArgAnnotation(const char* member, const char* arg, const qcc::String& name, const qcc::String& value)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::MemberMap::iterator it = defs->members.find(std::string(member));
    if (it == defs->members.end()) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }

    Member& m = it->second;
    pair<qcc::String, qcc::String> item(qcc::String(arg), name);
    std::pair<ArgumentAnnotations::iterator, bool> ret = m.argumentAnnotations->insert(ArgumentAnnotations::value_type(item, value));
    return (ret.second || ((ret.first->first.first.compare(arg) == 0) && (ret.first->first.second == name))) ? ER_OK : ER_BUS_ANNOTATION_ALREADY_EXISTS;
}

bool InterfaceDescription::GetArgAnnotation(const char* member, const char* arg, const qcc::String& name, qcc::String& value) const
{
    Definitions::MemberMap::iterator it = defs->members.find(std::string(member));
    if (it == defs->members.end()) {
        return false;
    }

    Member& m = it->second;
    return m.GetArgAnnotation(arg, name, value);
}

const char* InterfaceDescription::Translate(const char* toLanguage, const char* text, qcc::String& buffer, Translator* translator) const
{
    if (NULL == text) {
        return NULL;
    }

    if (translator) {
        qcc::String bestLanguage;
        translator->GetBestLanguage(toLanguage, defs->languageTag, bestLanguage);
        const char* ret = translator->Translate(defs->languageTag.c_str(), bestLanguage.c_str(), text, buffer);
        if (ret && ret[0] != '\0') {
            return ret;
        }
    }

    if (text && text[0] != '\0') {
        return text;
    }

    return NULL;
}

void InterfaceDescription::SetDescriptionTranslator(Translator* translator) {
    defs->translator = translator;
}

Translator* InterfaceDescription::GetDescriptionTranslator() const
{
    return defs->translator;
}

void InterfaceDescription::AppendDescriptionToAnnotations(AnnotationsMap& annotations, const char* description, Translator* translator) const
{
    if ((description == NULL) || (description[0] == '\0')) {
        return;
    }

    qcc::String docString("org.alljoyn.Bus.DocString");
    qcc::String localDescription(description);

    /* Append description of all languages */
    qcc::String language;
    if (translator != NULL) {
        size_t size = translator->NumTargetLanguages();
        for (size_t index = 0; index < size; index++) {
            translator->GetTargetLanguage(index, language);

            if (!language.empty()) {
                qcc::String buffer;
                const char* d = Translate(language.c_str(), description, buffer, translator);
                if ((d == NULL) || (d[0] == '\0')) {
                    continue;
                }
                localDescription = XmlElement::EscapeXml(d);
                qcc::String name = docString + "." + language;
                annotations[name] = localDescription;
            }
        }
    } else {
        /* Append with default language tag only if it hasn't been annotated */
        bool found = false;
        AnnotationsMap::const_iterator mit = annotations.begin();
        for (; mit != annotations.end(); ++mit) {
            if (localDescription == mit->second) {
                found = true;
                break;
            }
        }
        if (!found) {
            qcc::String name = docString;
            if (!defs->languageTag.empty()) {
                name += "." + defs->languageTag;
            }
            annotations[name] = localDescription;
        }
    }
}

void InterfaceDescription::AppendDescriptionToArgAnnotations(ArgumentAnnotations& argAnnotations, const char* argName, const char* description, Translator* translator) const
{
    if ((description == NULL) || (description[0] == '\0')) {
        return;
    }

    qcc::String docString("org.alljoyn.Bus.DocString");
    qcc::String localDescription(description);

    /* Append description of all languages */
    qcc::String language;
    if (translator != NULL) {
        size_t size = translator->NumTargetLanguages();
        for (size_t index = 0; index < size; index++) {
            translator->GetTargetLanguage(index, language);

            if (!language.empty()) {
                qcc::String buffer;
                const char* d = Translate(language.c_str(), description, buffer, translator);
                if ((d == NULL) || (d[0] == '\0')) {
                    continue;
                }
                localDescription = XmlElement::EscapeXml(d);
                qcc::String name = docString + "." + language;

                pair<qcc::String, qcc::String> item(qcc::String(argName), name);
                argAnnotations.insert(ArgumentAnnotations::value_type(item, localDescription));
            }
        }
    } else {
        /* Append with default language tag only if it hasn't been annotated */
        bool found = false;
        ArgumentAnnotations::const_iterator ait = argAnnotations.begin();
        for (; ait != argAnnotations.end(); ++ait) {
            if (localDescription == ait->second) {
                found = true;
                break;
            }
        }
        if (!found) {
            qcc::String name = docString;
            if (!defs->languageTag.empty()) {
                name += "." + defs->languageTag;
            }
            pair<qcc::String, qcc::String> item(qcc::String(argName), name);
            argAnnotations.insert(ArgumentAnnotations::value_type(item, localDescription));
        }
    }
}
}
