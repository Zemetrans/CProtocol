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

#ifndef _ALLJOYN_TESTSECUREAPPLICATION_H
#define _ALLJOYN_TESTSECUREAPPLICATION_H

#include <string>
#include <vector>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/AuthListener.h>
#include "TestSecurityManager.h"
#include "InMemoryKeyStore.h"

using namespace std;
using namespace ajn;
using namespace qcc;


#define DEFAULT_TEST_PORT 12345
#define TEST_INTERFACE "test.interface"
#define TEST_PROP_NAME "test_property"
#define TEST_PROP_NAME2 "other_test_property"
#define TEST_METHOD_NAME "test_method"
#define TEST_SIGNAL_NAME "test_signal"
#define DEFAULT_TEST_OBJ_PATH "/default/test/object/path"
#define TEST_SIGNAL_MATCH_RULE  "type='signal',interface='" TEST_INTERFACE "',member='" TEST_SIGNAL_NAME "'"

class TestSecureApplication :
    private SessionPortListener,
    private SessionListener {
  public:
    TestSecureApplication(const char* name) : testObj(NULL), bus(name), authListener(this), appName(name), pcl(this), joinSessionEvent(nullptr), endManagementEvent(nullptr)
    {
        bus.RegisterKeyStoreListener(keyStoreListener);
    }

    ~TestSecureApplication();

    QStatus Init(TestSecurityManager& tsm);

    QStatus HostSession(SessionPort port = DEFAULT_TEST_PORT, bool multipoint = false);

    QStatus JoinSession(TestSecureApplication& sessionHost, SessionId& sessionId, SessionPort port = DEFAULT_TEST_PORT, bool multipoint = false);

    QStatus SetAnyTrustedUserPolicy(TestSecurityManager& tsm, uint8_t actionMask, const char* interfaceName = TEST_INTERFACE);
    QStatus SetPolicy(TestSecurityManager& tsm, PermissionPolicy& newPolicy);

    QStatus UpdateManifest(TestSecurityManager& tsm, uint8_t actionMask, const char* interfaceName = TEST_INTERFACE);
    QStatus UpdateManifest(TestSecurityManager& tsm, const PermissionPolicy::Acl& manifest);

    ProxyBusObject* GetProxyObject(TestSecureApplication& host, SessionId sessionId, const char* objPath = DEFAULT_TEST_OBJ_PATH);

    QStatus UpdateTestProperty(bool newState);

    /** Sends a signal using SESSION_ID_ALL_HOSTED */
    QStatus SendSignal(bool value);
    /** Send a unicast signal to destination. */
    QStatus SendSignal(bool value, TestSecureApplication& destination);

    BusAttachment& GetBusAttachement()
    {
        return bus;
    }

    int GetCurrentGetPropertyCount() {
        return testObj ? testObj->getCount : -1;
    }

    /* Methods used to secure one or all connections, then wait for all the involved peers to finish authentication */
    QStatus SecureConnection(TestSecureApplication& peer, bool forceAuth = false);

    QStatus SecureConnectionAllSessions(TestSecureApplication& peer, bool forceAuth = false);
    QStatus SecureConnectionAllSessionsAsync(TestSecureApplication& peer, bool forceAuth = false);
    QStatus SecureConnectionAllSessionsCommon(bool async, TestSecureApplication& peer, bool forceAuth);

    QStatus SecureConnectionAllSessions(TestSecureApplication& peer1, TestSecureApplication& peer2, bool forceAuth = false);
    QStatus SecureConnectionAllSessionsAsync(TestSecureApplication& peer1, TestSecureApplication& peer2, bool forceAuth = false);
    QStatus SecureConnectionAllSessionsCommon(bool async, TestSecureApplication& peer1, TestSecureApplication& peer2, bool forceAuth);

  private:
    class TestObject : public BusObject {
      public:
        TestObject(BusAttachment& bus, const char* path = DEFAULT_TEST_OBJ_PATH) : BusObject(path),
            ba(&bus), initialized(false), state(false), getCount(0)
        {
        }

        ~TestObject()
        {
            if (initialized) {
                ba->UnregisterBusObject(*this);
            }
        }

        QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
        {
            cout << "TestObject::Get(" <<  propName << ", " << ifcName << ")" << endl;
            if (strcmp(ifcName, TEST_INTERFACE) == 0 && strcmp(propName, TEST_PROP_NAME) == 0) {
                val.Set("b", state);
                getCount++;
                return ER_OK;
            }
            if (strcmp(ifcName, TEST_INTERFACE) == 0 && strcmp(propName, TEST_PROP_NAME2) == 0) {
                val.Set("b", state);
                getCount++;
                return ER_OK;
            }
            return ER_BUS_NO_SUCH_PROPERTY;
        }

        void TestMethod(const InterfaceDescription::Member* member, Message& msg)
        {
            QCC_UNUSED(member);
            QCC_UNUSED(msg);
        }

        QStatus Init() {
            const InterfaceDescription* testIntf = ba->GetInterface(TEST_INTERFACE);
            if (testIntf == NULL) {
                cerr << "SecureApplication::TestObject::Init failed " << __LINE__ << endl;
                return ER_FAIL;
            }
            QStatus status = AddInterface(*testIntf, ANNOUNCED);
            if (ER_OK != status) {
                cerr << "SecureApplication::TestObject::Init failed " << __LINE__ << endl;
                return status;
            }

            /* Register the method handlers with the object */
            const MethodEntry methodEntries[] = {
                { testIntf->GetMember(TEST_METHOD_NAME), static_cast<MessageReceiver::MethodHandler>(&TestObject::TestMethod) },
            };
            status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
            if (ER_OK != status) {
                cerr << "SecureApplication::TestObject::Init failed " << __LINE__ << endl;
                return status;
            }
            status = ba->RegisterBusObject(*this, true);
            if (ER_OK == status) {
                initialized = true;
            }
            return status;
        }
      private:
        BusAttachment* ba;
        bool initialized;
      public:
        bool state;
        int getCount;
    };

    class MyAuthListener : public DefaultECDHEAuthListener {
      public:
        MyAuthListener(TestSecureApplication* app) : m_app(app) { };
        virtual ~MyAuthListener() { };

        virtual void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success);

      private:
        TestSecureApplication* m_app;
    };

    class MyPermissionConfigurationListener : public PermissionConfigurationListener {
      public:
        MyPermissionConfigurationListener(TestSecureApplication* app) : m_app(app) { };
        virtual ~MyPermissionConfigurationListener() { };

        virtual void EndManagement()
        {
            PermissionConfigurationListener::EndManagement();

            if (m_app != nullptr) {
                m_app->EndManagementCompleteCallback();
            }
        }

      private:
        TestSecureApplication* m_app;
    };

    virtual void SessionLost(SessionId sessionId, SessionLostReason reason);
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts);
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner);

    /* Support for waiting to complete authentication */
    void DeleteAllAuthenticationEvents();
    void AddAuthenticationEvent(const qcc::String& peerName, Event* authEvent);
    QStatus WaitAllAuthenticationEvents(uint32_t timeout);
    void AuthenticationCompleteCallback(qcc::String peerName, bool success);

    /* Support for waiting to complete JoinSession */
    void RemoveJoinSessionEvent();
    void SetJoinSessionEvent(qcc::Event* authEvent);
    QStatus WaitJoinSessionEvent(uint32_t timeout);
    void JoinSessionCompleteCallback();

    /* Support for waiting for EndManagement */
    void RemoveEndManagementEvent();
    void SetEndManagementEvent(qcc::Event* event);
    QStatus WaitEndManagementEvent(uint32_t timeout);
    void EndManagementCompleteCallback();

    vector<SessionId> joinedSessions;
    vector<SessionId> hostedSessions;
    TestObject* testObj;
    Mutex sessionLock;
    BusAttachment bus;
    MyAuthListener authListener;
    InMemoryKeyStoreListener keyStoreListener;
    qcc::String appName;
    MyPermissionConfigurationListener pcl;
    std::map<qcc::String, qcc::Event*> authEvents;
    qcc::Event* joinSessionEvent;
    qcc::Event* endManagementEvent;
};

#endif /* _ALLJOYN_TESTSECUREAPPLICATION_H */
