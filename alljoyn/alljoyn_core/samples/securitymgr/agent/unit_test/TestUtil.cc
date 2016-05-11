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
#include "TestUtil.h"

#include <stdlib.h>

#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/Environ.h>

#include <alljoyn/securitymgr/Util.h>

#include "SQLStorageConfig.h"

using namespace ajn;
using namespace ajn::securitymgr;
using namespace std;

/** @file TestUtil.cc */

namespace secmgr_tests {
TestApplicationListener::TestApplicationListener(Condition& _sem,
                                                 Mutex& _lock,
                                                 Condition& _errorSem,
                                                 Mutex& _errorLock,
                                                 Condition& _manifestSem,
                                                 Mutex& _manifestLock) :
    sem(_sem), lock(_lock),
    errorSem(_errorSem), errorLock(_errorLock),
    manifestSem(_manifestSem), manifestLock(_manifestLock)
{
}

void TestApplicationListener::OnApplicationStateChange(const OnlineApplication* old,
                                                       const OnlineApplication* updated)
{
    const OnlineApplication* info = updated ? updated : old;
    cout << "TAL>> Old Application info = " << (old == nullptr ? "null" : old->ToString().c_str()) << endl;
    cout << "TAL>> New Application info = " << (updated == nullptr ? "null" : updated->ToString().c_str()) << endl;
    lock.Lock();
    events.push_back(*info);
    sem.Broadcast();
    lock.Unlock();
}

string ToString(const SyncError* er)
{
    string result = "SyncError >>";
    result += " busName: " + er->app.busName;
    result += " type: " + string(SyncError::SyncErrorTypeToString(er->type));
    result += " status: " + string(QCC_StatusText(er->status));

    return result;
}

void TestApplicationListener::OnSyncError(const SyncError* er)
{
    cout << ToString(er) << endl;
    errorLock.Lock();
    syncErrors.push_back(*er);
    errorSem.Broadcast();
    errorLock.Unlock();
}

string ManifestUpdateToString(const ManifestUpdate* update)
{
    string result = "ManifestUpdate >> ";
    result += update->app.busName + " requested ";
    result += to_string(update->additionalRules.GetRulesSize());
    result += " additional rules";
    return result;
}

static void GetDefaultStorageFilePath(string& storageFilePath)
{
#if !defined(QCC_OS_GROUP_WINDOWS)
    storageFilePath = "/tmp";
#else
    /* Same path is returned by qcc::GetHomeDir() too */
    storageFilePath = Environ::GetAppEnviron()->Find("LOCALAPPDATA").c_str();
    if (storageFilePath.empty()) {
        storageFilePath = Environ::GetAppEnviron()->Find("USERPROFILE").c_str();
    }
#endif

    storageFilePath += "/secmgr.db";
}

void TestApplicationListener::OnManifestUpdate(const ManifestUpdate* manifestUpdate)
{
    cout << ManifestUpdateToString(manifestUpdate) << endl;
    manifestLock.Lock();
    manifestUpdates.push_back(*manifestUpdate);
    manifestSem.Broadcast();
    manifestLock.Unlock();
}

BasicTest::BasicTest() :
    ownBus(nullptr), proxyObjectManager(nullptr), tal(nullptr),
    secMgr(nullptr), ba(nullptr), storage(nullptr), ca(nullptr), pg(nullptr)
{
}

void BasicTest::SetUp()
{
    string storage_path = Environ::GetAppEnviron()->Find(STORAGE_FILEPATH_KEY).c_str();

    if (storage_path.empty()) {
        GetDefaultStorageFilePath(storage_path);
        assert(!storage_path.empty());
        Environ::GetAppEnviron()->Add(STORAGE_FILEPATH_KEY, storage_path.c_str());
    }

    remove(storage_path.c_str());

    StorageFactory& storageFac = StorageFactory::GetInstance();

    ba = new BusAttachment("testsecmgr", true);
    ASSERT_TRUE(ba != nullptr);
    ASSERT_EQ(ER_OK, ba->Start());
    ASSERT_EQ(ER_OK, ba->Connect());

    ba->RegisterAboutListener(testAboutListener);

    /* Passing nullptr into WhoImplements will listen for all About announcements */

    if (ER_OK != ba->WhoImplements(nullptr)) {
        printf("WhoImplements nullptr failed.\n");
    }

    ASSERT_EQ(ER_OK, storageFac.GetStorage(TEST_STORAGE_NAME, storage));
    ASSERT_EQ(ER_OK, storage->GetCaStorage(ca));

    GroupInfo adminGroup;
    storage->GetAdminGroup(adminGroup);
    pg = new PolicyGenerator(adminGroup);
}

void BasicTest::InitSecAgent()
{
    secAgentLock.Lock();
    SecurityAgentFactory& secFac = SecurityAgentFactory::GetInstance();
    ASSERT_EQ(ER_OK, secFac.GetSecurityAgent(GetAgentCAStorage(), secMgr, ba));
    ASSERT_NE(nullptr, secMgr);

    secMgr->SetClaimListener(&aa);
    tal = new TestApplicationListener(sem, lock, errorSem, errorLock, manifestSem, manifestLock);
    secMgr->RegisterApplicationListener(tal);
    secAgentLock.Unlock();
}

void BasicTest::RemoveSecAgent()
{
    secAgentLock.Lock();
    if (tal) {
        secMgr->UnregisterApplicationListener(tal);
        delete tal;
        tal = nullptr;
    }

    secMgr = nullptr;
    secAgentLock.Unlock();
}

bool BasicTest::UpdateLastAppInfo(OnlineApplication& lastAppInfo, const OnlineApplication& needed, bool useBusName)
{
    lock.Lock();
    bool found = false;
    vector<OnlineApplication>::iterator it = tal->events.begin();
    while (it != tal->events.end()) {
        if (useBusName ? needed.busName == it->busName : needed.keyInfo == it->keyInfo) {
            lastAppInfo = *it;
            tal->events.erase(it);
            found = true;
            break;
        }
        it++;
    }
    lock.Unlock();
    return found;
}

QStatus BasicTest::CreateProxyObjectManager()
{
    if (nullptr == proxyObjectManager) {
        ownBus = new BusAttachment("ownsecmgrtest");
        QStatus status = ownBus->Start();
        if (ER_OK != status) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return status;
        }
        status = ownBus->Connect();
        if (ER_OK != status) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return status;
        }
        proxyObjectManager = shared_ptr<ProxyObjectManager>(new ProxyObjectManager(ownBus));
        status =
            ownBus->EnablePeerSecurity(KEYX_ECDHE_NULL " " ECDHE_KEYX, &proxyObjectManager->listener, nullptr, true);
        PermissionPolicy::Rule rule;
        rule.SetInterfaceName("*");
        PermissionPolicy::Rule::Member member;
        member.SetMemberName("*");
        member.SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
        member.SetActionMask(
            PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_PROVIDE |
            PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rule.SetMembers(1, &member);
        ownBus->GetPermissionConfigurator().SetPermissionManifest(&rule, 1);
        IdentityInfo id;
        id.name = "basictest";
        storage->StoreIdentity(id);
        OnlineApplication me;
        me.busName = ownBus->GetUniqueName().c_str();
        if (!WaitForState(me, PermissionConfigurator::CLAIMABLE, SYNC_UNKNOWN, true)) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return ER_FAIL;
        }
        {
            ProxyObjectManager::ManagedProxyObject mpo(me);
            status = proxyObjectManager->GetProxyObject(mpo, ProxyObjectManager::ECDHE_NULL);
            if (ER_OK != status) {
                cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
                return status;
            }
            ECCPublicKey pk;
            status = mpo.GetPublicKey(pk);
            if (ER_OK != status) {
                cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
                return status;
            }
            me.keyInfo.SetPublicKey(&pk);
        }
        status = secMgr->Claim(me, id);
        if (ER_OK != status) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return status;
        }
        status = ownBus->EnablePeerSecurity(ECDHE_KEYX, &proxyObjectManager->listener);
        if (ER_OK != status) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return status;
        }
        GroupInfo adminGroup;
        status = storage->GetAdminGroup(adminGroup);
        if (ER_OK != status) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return status;
        }
        status = storage->StoreGroup(adminGroup);
        if (ER_OK != status) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return status;
        }
        status = storage->InstallMembership(me, adminGroup);
        if (ER_OK != status) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return status;
        }
        if (!WaitForUpdatesCompleted(me)) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return ER_FAIL;
        }
        PermissionPolicy p;
        PermissionPolicy::Acl pacl;
        PermissionPolicy::Peer peers[2];
        peers[1].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
        peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
        shared_ptr<AgentCAStorage> ca;
        storage->GetCaStorage(ca);
        KeyInfoNISTP256 pk;
        ca->GetCaPublicKeyInfo(pk);
        peers[0].SetKeyInfo(&pk);
        pacl.SetPeers(2, peers);
        pacl.SetRules(1, &rule);
        p.SetAcls(1, &pacl);
        status = storage->UpdatePolicy(me, p);
        if (ER_OK != status) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return status;
        }
        if (!WaitForUpdatesCompleted(me)) {
            cerr << "Failure in " << __FILE__ << "@" << __LINE__ << endl;
            return ER_FAIL;
        }
    }
    return ER_OK;
}

bool BasicTest::WaitForState(const OnlineApplication& appInfoNeeded, PermissionConfigurator::ApplicationState newState,
                             ApplicationSyncState syncState, bool useBusName)
{
    if (SYNC_UNKNOWN == syncState) {
        switch (newState) {
        case PermissionConfigurator::NOT_CLAIMABLE:
        case PermissionConfigurator::CLAIMABLE:
            syncState = SYNC_UNMANAGED;
            break;

        case PermissionConfigurator::CLAIMED:
        case PermissionConfigurator::NEED_UPDATE:
            syncState = SYNC_OK;
            break;
        }
    }

    lock.Lock();
    printf("\nWaitForState: waiting for event(s) ...\n");
    //Prior to entering this function, the test should have taken an action which leads to one or more events.
    //These events are handled in a separate thread.
    OnlineApplication appInfo;
    do {
        if (tal && UpdateLastAppInfo(appInfo, appInfoNeeded, useBusName)) { //if no event is in the queue, we will return immediately
            printf("WaitForState: Checking event ... ");
            if ((newState == appInfo.applicationState)
                && (syncState == appInfo.syncState)) {
                printf("ok\n");
                lock.Unlock();
                return true;
            }
            printf("not ok, waiting/checking for next event\n");
        } else {
            QStatus status = sem.TimedWait(lock, 10000);
            if (ER_OK != status) {
                printf("timeout- failing test - %i\n", status);
                break;
            }
            assert(tal->events.size()); // assume TimedWait returns != ER_OK in case of timeout
        }
    } while (true);
    printf("WaitForState failed.\n");
    printf("\tClaimableState: expected = %s, got %s\n", PermissionConfigurator::ToString(
               newState), PermissionConfigurator::ToString(appInfo.applicationState));
    printf("\t Busname appInfo.busName (%s)\n", appInfo.busName.c_str());
    printf("\t SyncState : expected = %s, got %s\n",
           ToString(syncState), ToString(appInfo.syncState));

    lock.Unlock();
    return false;
}

bool BasicTest::WaitForEvents(size_t numOfEvents)
{
    lock.Lock();
    printf("\nWaitForState: waiting for %zu event(s) ...\n", numOfEvents);
    //Prior to entering this function, the test should have taken an action which leads to one or more events.
    //These events are handled in a separate thread.
    do {
        if (tal && (tal->events.size() == numOfEvents)) { //if no event is in the queue, we will return immediately
            break;
        } else {
            QStatus status = sem.TimedWait(lock, 10000);
            if (ER_OK != status) {
                printf("timeout- failing test - %i\n", status);
                break;
            }
            assert(tal->events.size()); // assume TimedWait returns != ER_OK in case of timeout
        }
    } while (true);
    lock.Unlock();
    return false;
}

bool BasicTest::CheckRemotePolicy(const OnlineApplication& app, PermissionPolicy& expected)
{
    bool result = true;
    CreateProxyObjectManager();

    printf("Checking remote policy ... ");
    ProxyObjectManager::ManagedProxyObject mngdProxy(app);
    QStatus status = proxyObjectManager->GetProxyObject(mngdProxy);
    if (ER_OK != status) {
        printf("failed to connect to the application\n");
        return false;
    }

    PermissionPolicy remote;
    status = mngdProxy.GetPolicy(remote);
    if (ER_OK != status) {
        printf("failed to GetPolicy\n");
        return false;
    }

    if (expected != remote) {
        printf("mismatching remote policy: expected %s, got %s\n",
               expected.ToString().c_str(),
               remote.ToString().c_str());
        return false;
    }

    printf("ok\n");
    return result;
}

bool BasicTest::CheckStoredPolicy(const OnlineApplication& app, PermissionPolicy& expected)
{
    bool result = true;
    QStatus status = ER_OK;
    CreateProxyObjectManager();

    printf("Checking stored policy ... ");
    PermissionPolicy stored;
    status = ca->GetPolicy(app, stored);
    if (ER_OK != status) {
        printf("failed to GetPolicy\n");
        return false;
    }

    if (expected != stored) {
        printf("mismatching stored policy: expected %s, got %s\n",
               expected.ToString().c_str(),
               stored.ToString().c_str());
        return false;
    }

    printf("ok\n");
    return result;
}

bool BasicTest::CheckPolicy(const OnlineApplication& app, PermissionPolicy& expected)
{
    return CheckRemotePolicy(app, expected) && CheckStoredPolicy(app, expected);
}

QStatus BasicTest::Reset(const OnlineApplication& app)
{
    CreateProxyObjectManager();
    ProxyObjectManager::ManagedProxyObject mngdProxy(app);
    QStatus status = proxyObjectManager->GetProxyObject(mngdProxy);
    if (ER_OK != status) {
        printf("failed to connect to the application\n");
        return status;
    }
    return mngdProxy.Reset();
}

QStatus BasicTest::GetMembershipSummaries(const OnlineApplication& app, vector<MembershipSummary>& summaries)
{
    CreateProxyObjectManager();
    ProxyObjectManager::ManagedProxyObject mngdProxy(app);
    QStatus status = proxyObjectManager->GetProxyObject(mngdProxy);
    if (ER_OK != status) {
        printf("failed to connect to the application\n");
        return status;
    }
    return mngdProxy.GetMembershipSummaries(summaries);
}

QStatus BasicTest::GetPolicyVersion(const OnlineApplication& app, uint32_t& version)
{
    CreateProxyObjectManager();
    ProxyObjectManager::ManagedProxyObject mngdProxy(app);
    QStatus status = proxyObjectManager->GetProxyObject(mngdProxy);
    if (ER_OK != status) {
        printf("failed to connect to the application\n");
        return status;
    }
    return mngdProxy.GetPolicyVersion(version);
}

QStatus BasicTest::GetIdentity(const OnlineApplication& app, IdentityCertificateChain& idCertChain)
{
    CreateProxyObjectManager();
    ProxyObjectManager::ManagedProxyObject mngdProxy(app);
    QStatus status = proxyObjectManager->GetProxyObject(mngdProxy);
    if (ER_OK != status) {
        printf("failed to connect to the application\n");
        return status;
    }
    return mngdProxy.GetIdentity(idCertChain);
}

QStatus BasicTest::GetClaimCapabilities(const OnlineApplication& app,
                                        PermissionConfigurator::ClaimCapabilities& claimCaps,
                                        PermissionConfigurator::ClaimCapabilityAdditionalInfo& claimCapInfo)
{
    CreateProxyObjectManager();
    ProxyObjectManager::ManagedProxyObject mngdProxy(app);
    QStatus status = proxyObjectManager->GetProxyObject(mngdProxy, ProxyObjectManager::ECDHE_NULL);
    if (ER_OK != status) {
        printf("failed to connect to the application\n");
        return status;
    }
    return mngdProxy.GetClaimCapabilities(claimCaps, claimCapInfo);
}

bool BasicTest::CheckDefaultPolicy(const OnlineApplication& app)
{
    bool result = true;
    CreateProxyObjectManager();

    printf("Retrieving default policy ... ");
    ProxyObjectManager::ManagedProxyObject mngdProxy(app);
    QStatus status = proxyObjectManager->GetProxyObject(mngdProxy);
    if (ER_OK != status) {
        printf("failed to connect to the application\n");
        return false;
    }
    PermissionPolicy defaultPolicy;
    status = mngdProxy.GetDefaultPolicy(defaultPolicy);
    if (ER_OK != status) {
        printf("failed to GetDefaultPolicy\n");
        return false;
    }
    printf("ok\n");

    if (!CheckRemotePolicy(app, defaultPolicy)) {
        return false;
    }

    printf("Retrieving stored policy ... ");
    PermissionPolicy stored;
    status = ca->GetPolicy(app, stored);
    QStatus expectedStatus = ER_END_OF_DATA;
    if (expectedStatus != status) {
        printf("mismatching status: expected %i, got %i\n", expectedStatus, status);
        return false;
    }
    printf("ok\n");

    return result;
}

bool BasicTest::CheckRemoteIdentity(const OnlineApplication& app,
                                    IdentityInfo& expected,
                                    ajn::securitymgr::Manifest& expectedManifest,
                                    IdentityCertificate& remoteIdentity,
                                    ajn::securitymgr::Manifest& remoteManifest)
{
    CreateProxyObjectManager();
    printf("Checking remote identity ... \n");
    ProxyObjectManager::ManagedProxyObject mngdProxy(app);
    QStatus status = proxyObjectManager->GetProxyObject(mngdProxy);
    if (ER_OK != status) {
        printf("failed to connect to application\n");
        return false;
    }

    IdentityCertificateChain remoteIdentityChain;

    status = mngdProxy.GetIdentity(remoteIdentityChain);
    if (ER_OK != status) {
        printf("failed to GetIdentity\n");
        return false;
    }

    remoteIdentity = remoteIdentityChain[0];

    string expectedAlias = expected.guid.ToString().c_str();
    string remoteAlias = remoteIdentity.GetAlias().c_str();
    if (expectedAlias != remoteAlias) {
        printf("mismatching alias: expected %s, got %s\n", expectedAlias.c_str(), remoteAlias.c_str());
        return false;
    }

    status = mngdProxy.GetManifest(remoteManifest);
    if (ER_OK != status) {
        printf("failed to GetManifest\n");
        return false;
    }

    if (!remoteManifest.TemplateEquals(expectedManifest)) {
        printf("mismatching remote manifest: expected %s, got %s\n",
               expectedManifest.ToString().c_str(),
               remoteManifest.ToString().c_str());
        return false;
    }

    printf("ok\n");
    return true;
}

bool BasicTest::CheckIdentity(const OnlineApplication& app,
                              IdentityInfo& expected,
                              ajn::securitymgr::Manifest& expectedManifest)
{
    IdentityCertificate remoteIdentity;
    ajn::securitymgr::Manifest remoteManifest;

    if (!CheckRemoteIdentity(app, expected, expectedManifest,
                             remoteIdentity, remoteManifest)) {
        return false;
    }

    printf("Checking stored identity ... ");
    QStatus status = ER_OK;

    IdentityCertificateChain storedIdCerts;
    ajn::securitymgr::Manifest storedManifest;
    status = ca->GetIdentityCertificatesAndManifest(app, storedIdCerts, storedManifest);
    if (ER_OK != status) {
        printf("failed to GetIdentityCertificateAndManifest\n");
        return false;
    }

    String storedDer;
    status = storedIdCerts[0].EncodeCertificateDER(storedDer);
    if (ER_OK != status) {
        printf("failed to encode stored certificate\n");
        return false;
    }

    String remoteDer;
    status = remoteIdentity.EncodeCertificateDER(remoteDer);
    if (ER_OK != status) {
        printf("failed to encode remote certificate\n");
        return false;
    }

    if (storedDer != remoteDer) {
        printf("mismatching encoded certificates\n");
        return false;
    }

    if (!remoteManifest.TemplateEquals(expectedManifest)) {
        printf("mismatching stored manifest: expected %s, got %s\n",
               expectedManifest.ToString().c_str(),
               storedManifest.ToString().c_str());
        return false;
    }

    printf("ok\n");
    return true;
}

bool BasicTest::CheckMemberships(const OnlineApplication& app, vector<GroupInfo> expected)
{
    printf("Checking remote memberships ... ");
    vector<MembershipSummary> remote;
    QStatus status = GetMembershipSummaries(app, remote);
    if (ER_OK != status) {
        printf("failed to GetMembershipSummaries\n");
        return false;
    }

    size_t expectedSize = expected.size();
    size_t remoteSize = remote.size();
    if (expectedSize != remoteSize) {
        printf("mismatching size: expected %lu, got %lu\n",
               (unsigned long)expectedSize,
               (unsigned long)remoteSize);
        return false;
    }

    vector<MembershipCertificateChain> stored;
    status = ca->GetMembershipCertificates(app, stored);
    if (ER_OK != status) {
        printf("failed to GetMembershipCertificates\n");
        return status;
    }

    vector<GroupInfo>::size_type e;
    vector<MembershipCertificate>::size_type s;
    vector<MembershipSummary>::size_type r;
    // for each expected membership
    for (e = 0; e != expected.size(); e++) {
        // find serial number from storage
        string serial;
        for (s = 0; s != stored.size(); s++) {
            if (stored[s][0].GetGuild() == expected[e].guid) {
                serial = string((const char*)stored[s][0].GetSerial(),
                                stored[s][0].GetSerialLen());
                break;
            }
        }
        if (serial.empty()) {
            printf("could not determine serial number for %s\n",
                   expected[e].name.c_str());
            return false;
        }

        // find serial number in remotes
        for (r = 0; r != remote.size(); r++) {
            if (remote[r].serial == serial) {
                break;
            }
        }
        if (r == remote.size()) {
            printf("could not find remote certificate for %s\n",
                   expected[e].name.c_str());
            return false;
        }

        // remove from remotes
        remote.erase(remote.begin() + r);
    }

    if (remote.size() != 0) {
        printf("found unexpected remote certificate\n");
        return false;
    }

    printf("ok\n");
    return true;
}

bool BasicTest::CheckSyncState(const OnlineApplication& appinfo, ApplicationSyncState expected)
{
    printf("Checking sync state in security agent ... ");
    OnlineApplication check;
    check.keyInfo = appinfo.keyInfo;
    QStatus status = secMgr->GetApplication(check);
    if (ER_OK != status) {
        printf("failed to GetApplication\n");
        return false;
    }

    ApplicationSyncState actual = check.syncState;
    if (expected != actual) {
        printf("unexpected syncState: expected %s , got %s\n",
               ToString(expected), ToString(actual));
        return false;
    }

    printf("ok\n");
    return true;
}

bool BasicTest::WaitForUpdatesCompleted(const OnlineApplication& app)
{
    printf("Waiting for updates completed ... ");
    bool result = true;

    result = WaitForState(app, PermissionConfigurator::CLAIMED, SYNC_PENDING);
    if (!result) {
        return result;
    }

    result = WaitForState(app, PermissionConfigurator::CLAIMED, SYNC_OK);
    if (!result) {
        return result;
    }

    result = CheckSyncState(app, SYNC_OK);

    return result;
}

bool BasicTest::WaitForSyncError(SyncErrorType type, QStatus status)
{
    printf("Waiting for sync error ... ");
    bool result = false;

    errorLock.Lock();
    do {
        if (tal && tal->syncErrors.size()) {
            vector<SyncError>::iterator it = tal->syncErrors.begin();
            QStatus errorStatus = it->status;
            SyncErrorType errorType = it->type;
            result = ((errorType == type) && (errorStatus == status));
            tal->syncErrors.erase(it);
            errorLock.Unlock();

            if (result == true) {
                printf("ok\n");
            } else if (errorType != type) {
                printf("unexpected SyncErrorType: expected %s, got %s\n",
                       SyncError::SyncErrorTypeToString(type),
                       SyncError::SyncErrorTypeToString(errorType));
            } else if (errorStatus != status) {
                printf("unexpected Status: expected %s, got %s\n",
                       QCC_StatusText(status), QCC_StatusText(errorStatus));
            }

            return result;
        } else {
            QStatus status = errorSem.TimedWait(errorLock, 5000);
            if (ER_OK != status) {
                printf("timeout\n");
                break;
            }
        }
    } while (true);

    errorLock.Unlock();
    return false;
}

bool BasicTest::WaitForManifestUpdate(ManifestUpdate& manifestUpdate)
{
    printf("Waiting for manifest update ... ");

    manifestLock.Lock();
    do {
        if (tal && tal->manifestUpdates.size()) {
            vector<ManifestUpdate>::iterator it =
                tal->manifestUpdates.begin();
            manifestUpdate = *it;
            tal->manifestUpdates.erase(it);
            manifestLock.Unlock();

            printf("ok\n");

            return true;
        } else {
            QStatus status = manifestSem.TimedWait(manifestLock, 5000);
            if (ER_OK != status) {
                printf("timeout\n");
                break;
            }
        }
    } while (true);

    manifestLock.Unlock();
    return false;
}

bool BasicTest::CheckUnexpectedSyncErrors()
{
    printf("Checking for unexpected sync errors ... ");
    if (tal && tal->syncErrors.size() > 0) {
        printf("%lu unexpected sync error(s)\n",
               (unsigned long)tal->syncErrors.size());
        return false;
    }
    printf("ok\n");
    return true;
}

bool BasicTest::CheckUnexpectedManifestUpdates()
{
    printf("Checking for unexpected manifest updates ... ");
    if (tal && tal->manifestUpdates.size() > 0) {
        printf("%lu unexpected manifest update(s)\n",
               (unsigned long)tal->manifestUpdates.size());
        return false;
    }
    printf("ok\n");
    return true;
}

void BasicTest::TearDown()
{
    EXPECT_TRUE(CheckUnexpectedSyncErrors());
    EXPECT_TRUE(CheckUnexpectedManifestUpdates());

    proxyObjectManager = nullptr;

    if (nullptr != ownBus) {
        ownBus->Disconnect();
        ownBus->Stop();
        ownBus->ClearKeyStore();
        ownBus->Join();
        delete ownBus;
        ownBus = nullptr;
    }

    delete pg;
    pg = nullptr;

    if (secMgr) {
        RemoveSecAgent();
    }

    ba->UnregisterAboutListener(testAboutListener);

    ba->Disconnect();
    ba->Stop();
    ba->ClearKeyStore();
    ba->Join();

    delete ba;
    ba = nullptr;

    storage->Reset();
}

QStatus BasicTest::GetPublicKey(const TestApplication& app, OnlineApplication& appInfo)
{
    appInfo.busName = app.GetBusName();
    QStatus status = CreateProxyObjectManager();
    if (ER_OK != status) {
        EXPECT_EQ(ER_OK, status);
        return status;
    }
    ProxyObjectManager::ManagedProxyObject ta(appInfo);
    status = proxyObjectManager->GetProxyObject(ta, app.IsClaimed() ?
                                                ProxyObjectManager::ECDHE_DSA : ProxyObjectManager::ECDHE_NULL);
    if (ER_OK != status) {
        return status;
    }
    ECCPublicKey key;
    status = ta.GetPublicKey(key);
    if (ER_OK != status) {
        return status;
    }
    appInfo.keyInfo.SetPublicKey(&key);
    return status;
}
}
