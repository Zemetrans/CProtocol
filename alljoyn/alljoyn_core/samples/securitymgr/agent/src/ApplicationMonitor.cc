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

#include "ApplicationMonitor.h"

#include <qcc/Debug.h>

#include <alljoyn/securitymgr/Util.h>

#define QCC_MODULE "SECMGR_APPMON"

using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
ApplicationMonitor::ApplicationMonitor(BusAttachment* ba) :
    busAttachment(ba)
{
    QStatus status = ER_FAIL;

    if (nullptr == busAttachment) {
        QCC_LogError(status, ("nullptr busAttachment !"));
        return;
    }

    busAttachment->RegisterApplicationStateListener(*this);
}

ApplicationMonitor::~ApplicationMonitor()
{
    busAttachment->UnregisterApplicationStateListener(*this);
}

void ApplicationMonitor::State(const char* busName,
                               const KeyInfoNISTP256& publicKeyInfo,
                               PermissionConfigurator::ApplicationState state)
{
    SecurityInfo info;
    info.busName = busName;
    info.applicationState = state;
    info.keyInfo = publicKeyInfo;

    // Ignore signals of local security agent
    string localBusName = busAttachment->GetUniqueName().c_str();
    if (info.busName == localBusName) {
        return;
    }

    QCC_DbgPrintf(("Received ApplicationState !!!"));
    QCC_DbgPrintf(("busName = %s", info.busName.c_str()));
    QCC_DbgPrintf(("applicationState = %s", PermissionConfigurator::ToString(state)));

    appsMutex.Lock(__FILE__, __LINE__);

    map<string, SecurityInfo>::iterator it = applications.find(info.busName);
    if (it != applications.end()) {
        // known bus name
        SecurityInfo oldInfo = it->second;
        it->second = info;
        appsMutex.Unlock(__FILE__, __LINE__);
        NotifySecurityInfoListeners(&oldInfo, &info);
    } else {
        // new bus name
        applications[info.busName] = info;
        appsMutex.Unlock(__FILE__, __LINE__);
        NotifySecurityInfoListeners(nullptr, &info);
    }
}

vector<SecurityInfo> ApplicationMonitor::GetApplications() const
{
    appsMutex.Lock(__FILE__, __LINE__);

    if (!applications.empty()) {
        vector<SecurityInfo> apps;
        map<string, SecurityInfo>::const_iterator it = applications.begin();
        for (; it != applications.end(); ++it) {
            const SecurityInfo& app = it->second;
            apps.push_back(app);
        }
        appsMutex.Unlock(__FILE__, __LINE__);
        return apps;
    }
    appsMutex.Unlock(__FILE__, __LINE__);
    return vector<SecurityInfo>();
}

QStatus ApplicationMonitor::GetApplication(SecurityInfo& secInfo) const
{
    appsMutex.Lock(__FILE__, __LINE__);
    map<string, SecurityInfo>::const_iterator it  = applications.find(secInfo.busName);
    if (it != applications.end()) {
        secInfo = it->second;
        appsMutex.Unlock(__FILE__, __LINE__);
        return ER_OK;
    }
    appsMutex.Unlock(__FILE__, __LINE__);
    return ER_FAIL;
}

void ApplicationMonitor::RegisterSecurityInfoListener(SecurityInfoListener* al)
{
    if (nullptr != al) {
        appsMutex.Lock(__FILE__, __LINE__);
        securityListenersMutex.Lock(__FILE__, __LINE__);
        map<string, SecurityInfo>::const_iterator it = applications.begin();
        for (; it != applications.end(); ++it) {
            al->OnSecurityStateChange(nullptr, &(it->second));
        }
        listeners.push_back(al);
        securityListenersMutex.Unlock(__FILE__, __LINE__);
        appsMutex.Unlock(__FILE__, __LINE__);
    }
}

void ApplicationMonitor::UnregisterSecurityInfoListener(SecurityInfoListener* al)
{
    securityListenersMutex.Lock(__FILE__, __LINE__);
    vector<SecurityInfoListener*>::iterator it = find(listeners.begin(), listeners.end(), al);
    if (listeners.end() != it) {
        listeners.erase(it);
    }
    securityListenersMutex.Unlock(__FILE__, __LINE__);
}

void ApplicationMonitor::NotifySecurityInfoListeners(const SecurityInfo* oldSecInfo,
                                                     const SecurityInfo* newSecInfo)
{
    securityListenersMutex.Lock(__FILE__, __LINE__);
    for (size_t i = 0; i < listeners.size(); ++i) {
        listeners[i]->OnSecurityStateChange(oldSecInfo, newSecInfo);
    }
    securityListenersMutex.Unlock(__FILE__, __LINE__);
}
}
}
#undef QCC_MODULE
