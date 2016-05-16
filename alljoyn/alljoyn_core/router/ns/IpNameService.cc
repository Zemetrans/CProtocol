/**
 * @file
 * The lightweight name service implementation
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <qcc/Debug.h>

#include "BusInternal.h"
#include "IpNameService.h"
#include "IpNameServiceImpl.h"

#define QCC_MODULE "IPNS"

namespace ajn {

const uint16_t IpNameService::MULTICAST_MDNS_PORT = 5353;

static IpNameService* singletonIpNameService = NULL;

void IpNameService::Init()
{
    printf("Enter IpNameService::Init()\n");
    singletonIpNameService = new IpNameService();
    printf("Exit IpNameService::Init()\n");
}

void IpNameService::Shutdown()
{
    printf("Enter IpNameService::Shutdown()\n");
    delete singletonIpNameService;
    singletonIpNameService = NULL;
    printf("Exit IpNameService::Shutdown()\n");
}

IpNameService& IpNameService::Instance()
{
    printf("Enter and Exit IpNameService::Instance(). return *singletonIpNameService\n");
    return *singletonIpNameService;
}

IpNameService::IpNameService()
    : m_constructed(false), m_destroyed(false), m_refCount(0), m_pimpl(NULL)
{
    printf("Enter IpNameService::IpNameService()\n");
    //
    // AllJoyn is a multithreaded system.  Since the name service instance is
    // created on first use, the first use is in the Start() method of each
    // IP-related transport, and the starting of the transports happens on a
    // single thread, assume we are single-threaded here and don't do anything
    // fancy to prevent interleaving scenarios on the private implementation
    // constructor.
    //
    m_pimpl = new IpNameServiceImpl;
    m_constructed = true;
    printf("Exit IpNameService::IpNameService()\n");
}

IpNameService::~IpNameService()
{
    printf("Enter IpNameService::~IpNameService()\n");
    //
    // We get here (on Linux) because when main() returns, the function
    // __run_exit_handlers() is called which, in turn, calls the destructors of
    // all of the static objects in whatever order the linker has decided will
    // be best.
    //
    // For us, the troublesome object is going to be the BundledRouter.  It is
    // torn down by the call to its destructor which may happen before or after
    // the call to our destructor.  If we are destroyed first, the bundled
    // daemon may happily continue to call into the name service singleton since
    // it may have no idea that it is about to go away.
    //
    // Eventually, we want to explicitly control the construction and
    // destruction order, but for now we have to live with the insanity (and
    // complexity) of dealing with out-of-order destruction.
    //
    // The name service singleton is a static, so its underlying memory won't go
    // away.  So by marking the singleton as destroyed we will have a lasting
    // indication that it has become unusable in case some bozo (the bundled
    // daemon) accesses us during destruction time after we have been destroyed.
    //
    // The exit handlers are going to be called by the main thread (that
    // originally called the main function), so the destructors will be called
    // sequentially.  The interesting problem, though, is that the BundledRouter
    // is going to have possibly more than one transport running, and typically
    // each of those transports has multiple threads that could conceivably be
    // making name service calls.  So, while our destructor is being called by
    // the main thread, it is possible that other transport threads will also
    // be calling.  Like in hunting rabbits, We've got to be very, very careful.
    //
    // First, make sure no callbacks leak out of the private implementation
    // during this critical time by turning off ALL callbacks to ALL transports.
    //
    if (m_pimpl) {
        m_pimpl->ClearCallbacks();
        m_pimpl->ClearNetworkEventCallbacks();
    }

    //
    // Now we slam shut an entry gate so that no new callers can get through and
    // try to do things while we are destroying the private implementation.
    //
    m_destroyed = true;

    //
    // No new callers will now be let in, but there may be existing callers
    // rummaging around in the object.  If the private implemetation is not
    // careful about multithreading, it can begin destroying itself with
    // existing calls in progress.  Thankfully, that's not our problem here.
    // We assume it will do the right thing.
    //
    if (m_pimpl) {

        //
        // Deleting the private implementation must accomplish an orderly shutdown
        // with an impled Stop() and Join().
        //
        delete m_pimpl;
        m_pimpl = NULL;
    }
    printf("Exit IpNameService::~IpNameService()\n");
}

#define ASSERT_STATE(function) \
    { \
        QCC_ASSERT(m_constructed && "IpNameService::" # function "(): Singleton not constructed"); \
        QCC_ASSERT(!m_destroyed && "IpNameService::" # function "(): Singleton destroyed"); \
        QCC_ASSERT(m_pimpl && "IpNameService::" # function "(): Private impl is NULL"); \
    }

void IpNameService::Acquire(const qcc::String& guid, bool loopback)
{
    printf("Enter IpNameService::Acquire()\n");
    //
    // If the entry gate has been closed, we do not allow an Acquire to actually
    // acquire a reference.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::Acquire()\n");
        return;
    }

    //
    // The only way someone can get to us is if they call Instance() which will
    // cause the object to be constructed.
    //
    QCC_ASSERT(m_constructed && "IpNameService::Acquire(): Singleton not constructed");

    ASSERT_STATE("Acquire");
    int32_t refs = qcc::IncrementAndFetch(&m_refCount);
    if (refs == 1) {
        //
        // The first transport in gets to set the GUID and the loopback mode.
        // There should be only one GUID associated with a daemon process, so
        // this should never change; and loopback is a test mode set by a test
        // program pretending to be a single transport, so this is fine as well.
        //
        Init(guid, loopback);
        Start();
    }
    printf("Exit IpNameService::Acquire()\n");
}

void IpNameService::Release()
{
    printf("Enter IpNameService::Release()\n");
    //
    // If the entry gate has been closed, we do not allow a Release to actually
    // release a reference.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::Release()\n");
        return;
    }

    ASSERT_STATE("Release");
    int refs = qcc::DecrementAndFetch(&m_refCount);
    if (refs == 0) {
        //
        // The last transport to release its interest in the name service gets
        // pay the price for waiting for the service to exit.  Since we do a
        // Join(), this method is expected to be called out of a transport's
        // Join() so the price is expected.
        //
        Stop();
        Join();
    }
    printf("Exit IpNameService::Release()\n");
}

QStatus IpNameService::Start()
{
    printf("Enter IpNameService::Start()\n");
    //
    // If the entry gate has been closed, we do not allow a Start to actually
    // start anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Enter IpNameService::Start()\n");
        return ER_OK;
    }

    ASSERT_STATE("Start");
    printf("Enter IpNameService::Start()\n");
    return m_pimpl->Start();
}

bool IpNameService::Started()
{
    printf("Enter IpNameService::Started()\n");
    //
    // If the entry gate has been closed, we do not allow a Started to actually
    // test anything.  We just return false.  The singleton is going away and so
    // we assume we are running __run_exit_handlers() so main() has returned.
    // We are definitely shutting down, and the process is going to exit, so
    // tricking callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::Started(). Return false\n");
        return false;
    }

    ASSERT_STATE("Started");
    printf("Exit IpNameService::Started(). return m_pimpl -> Started()\n");
    return m_pimpl->Started();
}

QStatus IpNameService::Stop()
{
    printf("Enter IpNameService::Stop()\n");
    //
    // If the entry gate has been closed, we do not allow a Stop to actually
    // stop anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.  The destructor is going to handle
    // the actual Stop() and Join().
    //
    if (m_destroyed) {
        printf("Exit IpNameService::Stop(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("Stop");
    printf("Exit IpNameService::Stop(). m_pimpl -> Stop()\n");
    return m_pimpl->Stop();
}

QStatus IpNameService::Join()
{
    printf("Enter IpNameService::Join()\n");
    //
    // If the entry gate has been closed, we do not allow a Join to actually
    // join anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.  The destructor is going to handle
    // the actual Stop() and Join().
    //
    if (m_destroyed) {
        printf("Exit IpNameService::Join(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("Join");
    printf("Enter IpNameService::Join(). Return m_pimpl -> Join()\n");
    return m_pimpl->Join();
}

QStatus IpNameService::Init(const qcc::String& guid, bool loopback)
{
    printf("Enter IpNameService::Init()\n");
    //
    // If the entry gate has been closed, we do not allow an Init to actually
    // init anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::Init()\n");
        return ER_OK;
    }

    ASSERT_STATE("Init");
    printf("Exit IpNameService::Init(). Return m_pimpl -> Init(guid, loopback)\n");
    return m_pimpl->Init(guid, loopback);
}

void IpNameService::SetCallback(TransportMask transportMask,
                                Callback<void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint32_t>* cb)
{
    printf("Enter IpNameService::SetCallback()\n");
    //
    // If the entry gate has been closed, we do not allow a SetCallback to actually
    // set anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    // The gotcha is that if there is a valid callback set, and the caller is
    // now setting the callback to NULL to prevent any new callbacks, the caller
    // will expect that no callbacks will follow this call.  This is taken care
    // of by calling SetCallback(NULL) on the private implemtation BEFORE
    // setting m_destroyed in our destructor.  In other words, the possible set
    // to NULL has already been done.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::SetCallback()\n");
        return;
    }

    ASSERT_STATE("SetCallback");
    m_pimpl->SetCallback(transportMask, cb);
    printf("Exit IpNameService::SetCallback()\n");
}

void IpNameService::SetNetworkEventCallback(TransportMask transportMask,
                                            Callback<void, const std::map<qcc::String, qcc::IPAddress>&>* cb)
{
    printf("Enter IpNameService::SetNetworkEventCallback()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::SetNetworkEventCallback()\n");
        return;
    }

    ASSERT_STATE("SetNetworkEventCallback");
    m_pimpl->SetNetworkEventCallback(transportMask, cb);
    printf("Exit IpNameService::SetNetworkEventCallback()\n");
}

QStatus IpNameService::CreateVirtualInterface(const qcc::IfConfigEntry& entry)
{
    printf("Enter IpNameService::CreateVirtualInterface()\n");
    //
    // If the entry gate has been closed, we do not allow an OpenInterface to
    // actually open anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::CreateVirtualInterface(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("CreateVirtualInterface");
    printf("Exit IpNameService::CreateVirtualInterface(). m_pimpl -> CreateVirtualInterface(entry)\n");
    return m_pimpl->CreateVirtualInterface(entry);
}

QStatus IpNameService::DeleteVirtualInterface(const qcc::String& ifceName)
{
    printf("Enter IpNameService::DeleteVirtualInterface()\n");
    //
    // If the entry gate has been closed, we do not allow an OpenInterface to
    // actually open anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::DeleteVirtualInterface(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("DeleteVirtualInterface");
    printf("Exit IpNameService::DeleteVirtualInterface(). Return m_pimpl -> DeleteVirtualInterface(ifceName)\n");
    return m_pimpl->DeleteVirtualInterface(ifceName);
}

QStatus IpNameService::OpenInterface(TransportMask transportMask, const qcc::String& name)
{
    //
    // If the entry gate has been closed, we do not allow an OpenInterface to
    // actually open anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    printf("Enter IpNameService::OpenInterface()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::OpenInterface(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("OpenInterface");
    printf("Exit IpNameService::OpenInterface(). Return m_pimpl -> OpenInterface(transportMask, name)\n");
    return m_pimpl->OpenInterface(transportMask, name);
}

QStatus IpNameService::OpenInterface(TransportMask transportMask, const qcc::IPAddress& address)
{
    printf("Enter IpNameService::OpenInterface()\n");
    //
    // If the entry gate has been closed, we do not allow an OpenInterface to
    // actually open anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::OpenInterface(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("OpenInterface");
    printf("Exit IpNameService::OpenInterface(). return m_pibpl -> OpenInterface(transportMask, address)\n");
    return m_pimpl->OpenInterface(transportMask, address);
}

QStatus IpNameService::CloseInterface(TransportMask transportMask, const qcc::String& name)
{
    printf("Enter IpNameService::CloseInterface()\n");
    //
    // If the entry gate has been closed, we do not allow a CloseInterface to
    // actually close anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::CloseInterface(). return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("CloseInterface");
    printf("Exit IpNameService::CloseInterface(). return m_pimpl -> CloseInterface(transportMask, name)\n");
    return m_pimpl->CloseInterface(transportMask, name);
}

QStatus IpNameService::CloseInterface(TransportMask transportMask, const qcc::IPAddress& address)
{
    printf("Enter IpNameService::CloseInterface()\n");
    //
    // If the entry gate has been closed, we do not allow a CloseInterface to
    // actually close anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::CloseInterface(). Return ER_OK\n");
        return ER_OK;
    }
    ASSERT_STATE("CloseInterface");
    printf("Exit IpNameService::CloseInterface(). Return m_pimpl -> CloseInterface(transportMask, address)\n");
    return m_pimpl->CloseInterface(transportMask, address);
}

QStatus IpNameService::Enable(TransportMask transportMask,
                              const std::map<qcc::String, uint16_t>& reliableIPv4PortMap, uint16_t reliableIPv6Port,
                              const std::map<qcc::String, uint16_t>& unreliableIPv4PortMap, uint16_t unreliableIPv6Port,
                              bool enableReliableIPv4, bool enableReliableIPv6,
                              bool enableUnreliableIPv4, bool enableUnreliableIPv6)
{
    printf("Enter IpNameService::Enable()\n");
    //
    // If the entry gate has been closed, we do not allow an Enable to actually
    // enable anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        printf("Exit IpNameService::Enable(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("Enable");
    m_pimpl->Enable(transportMask, reliableIPv4PortMap, reliableIPv6Port, unreliableIPv4PortMap, unreliableIPv6Port,
                    enableReliableIPv4, enableReliableIPv6, enableUnreliableIPv4, enableUnreliableIPv6);
    printf("Exit IpNameService::Enable(). Return ER_OK\n");
    return ER_OK;
}

QStatus IpNameService::UpdateDynamicScore(TransportMask transportMask, uint32_t availableTransportConnections, uint32_t maximumTransportConnections,
                                          uint32_t availableTransportRemoteClients, uint32_t maximumTransportRemoteClients)
{
    printf("Enter IpNameService::UpdateDynamicScore()\n");
    ASSERT_STATE("UpdateDynamicScore");
    m_pimpl->UpdateDynamicScore(transportMask, availableTransportConnections, maximumTransportConnections, availableTransportRemoteClients, maximumTransportRemoteClients);
    printf("Exit IpNameService::UpdateDynamicScore(). Return ER_OK\n");
    return ER_OK;
}

QStatus IpNameService::Enabled(TransportMask transportMask,
                               std::map<qcc::String, uint16_t>& reliableIPv4PortMap, uint16_t& reliableIPv6Port,
                               std::map<qcc::String, uint16_t>& unreliableIPv4PortMap, uint16_t& unreliableIPv6Port)
{
    //
    // If the entry gate has been closed, we do not allow an Enabled to actually
    // do anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    printf("Enter IpNameService::Enabled()\n");
    if (m_destroyed) {
        reliableIPv6Port = unreliableIPv6Port = 0;
        reliableIPv4PortMap.clear();
        unreliableIPv4PortMap.clear();
        printf("Exit IpNameService::Enabled(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("Enabled");
    printf("Exot IpNameService::Enabled(). Return m_pimpl->Enabled(***)\n");
    return m_pimpl->Enabled(transportMask, reliableIPv4PortMap, reliableIPv6Port, unreliableIPv4PortMap, unreliableIPv6Port);
}

QStatus IpNameService::FindAdvertisement(TransportMask transportMask, const qcc::String& matching, TransportMask completeTransportMask)
{
    //
    // If the entry gate has been closed, we do not allow a FindAdvertisedName
    // to actually find anything.  The singleton is going away and so we assume
    // we are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    printf("Enter IpNameService::FindAdvertisement()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::FindAdvertisement(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("FindAdvertisement");
    printf("Exit IpNameService::FindAdvertisement(). Return m_pimpl -> FindAdvertisement(***)\n");
    return m_pimpl->FindAdvertisement(transportMask, matching, IpNameServiceImpl::ALWAYS_RETRY, completeTransportMask);
}

QStatus IpNameService::CancelFindAdvertisement(TransportMask transportMask, const qcc::String& matching, TransportMask completeTransportMask)
{
    printf("Enter IpNameService::CancelFindAdvertisement()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::CancelFindAdvertisement(). Return ER_OK\n");
        return ER_OK;
    }
    ASSERT_STATE("CancelFindAdvertisement");
    printf("Exit IpNameService::CancelFindAdvertisement(). return m_pimpl ->CancelFIndAdvertisement(***)\n");
    return m_pimpl->CancelFindAdvertisement(transportMask, matching, IpNameServiceImpl::ALWAYS_RETRY, completeTransportMask);
}

QStatus IpNameService::RefreshCache(TransportMask transportMask, const qcc::String& guid, const qcc::String& matching)
{
    printf("Enter IpNameService::RefreshCache()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::RefreshCache(). Return ER_OK\n");
        return ER_OK;
    }
    ASSERT_STATE("RefreshCache");
    printf("Exit IpNameService::RefreshCache(). Return m_pimpl ->RefreshCache(***)\n");
    return m_pimpl->RefreshCache(transportMask, guid, matching);
}

QStatus IpNameService::AdvertiseName(TransportMask transportMask, const qcc::String& wkn, bool quietly, TransportMask completeTransportMask)
{
    //
    // If the entry gate has been closed, we do not allow an AdvertiseName to
    // actually advertise anything.  The singleton is going away and so we
    // assume we are running __run_exit_handlers() so main() has returned.  We
    // are definitely shutting down, and the process is going to exit, so
    // tricking callers who may be temporarily running is okay.
    //
    printf("Enter IpNameService::AdvertiseName()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::AdvertiseName(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("AdvertiseName");
    printf("Exit IpNameService::AdvertiseName(). Return m_pimpl->AdvertiseName(***)\n");
    return m_pimpl->AdvertiseName(transportMask, wkn, quietly, completeTransportMask);
}

QStatus IpNameService::CancelAdvertiseName(TransportMask transportMask, const qcc::String& wkn, bool quietly, TransportMask completeTransportMask)
{
    //
    // If the entry gate has been closed, we do not allow a CancelAdvertiseName
    // to actually cancel anything.  The singleton is going away and so we
    // assume we are running __run_exit_handlers() so main() has returned.  We
    // are definitely shutting down, and the process is going to exit, so
    // tricking callers who may be temporarily running is okay.
    //
    printf("Enter IpNameService::CancelAdvertiseName()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::CancelAdvertiseName(). Return ER_OK\n");
        return ER_OK;
    }

    ASSERT_STATE("CancelAdvertiseName");
    printf("Exit IpNameService::CancelAdvertiseName(). Return m_pimpl ->CancelAdvertiseName(***)\n");
    return m_pimpl->CancelAdvertiseName(transportMask, wkn, quietly, completeTransportMask);
}

QStatus IpNameService::OnProcSuspend()
{
    printf("Enter IpNameService::OnProcSuspend()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::OnProcSuspend(). Return ER_OK\n");
        return ER_OK;
    }
    ASSERT_STATE("OnProcSuspend");
    printf("Exit IpNameService::OnProcSuspend(). Return m_pimpl->OnProcSuspend(***)\n");
    return m_pimpl->OnProcSuspend();
}

QStatus IpNameService::OnProcResume()
{
    printf("Enter IpNameService::OnProcResume()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::OnProcResume(). Return ER_OK\n");
        return ER_OK;
    }
    ASSERT_STATE("OnProcResume");
    printf("Exit IpNameService::OnProcResume(). Return m_pimpl->OnProcResume()\n");
    return m_pimpl->OnProcResume();
}

void IpNameService::RegisterListener(IpNameServiceListener& listener)
{
    printf("Enter IpNameService::RegisterListener()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::RegisterListener()\n");
        return;
    }
    ASSERT_STATE("RegisterListener");
    m_pimpl->RegisterListener(listener);
    printf("Exit IpNameService::RegisterListener()\n");
}

void IpNameService::UnregisterListener(IpNameServiceListener& listener)
{
    printf("Enter IpNameService::UnregisterListener()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::UnregisterListener()\n");
        return;
    }
    ASSERT_STATE("UnregisterListener");
    m_pimpl->UnregisterListener(listener);
    printf("Exit IpNameService::UnregisterListener()\n");
}

QStatus IpNameService::Ping(TransportMask transportMask, const qcc::String& guid, const qcc::String& name)
{
    printf("Enter IpNameService::Ping()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::Ping(). Return ER_OK\n");
        return ER_OK;
    }
    ASSERT_STATE("Ping");
    printf("Exit IpNameService::Ping(). Return m_pimpl->Ping(***)\n");
    return m_pimpl->Ping(transportMask, guid, name);
}

QStatus IpNameService::Query(TransportMask transportMask, MDNSPacket mdnsPacket)
{
    printf("Enter IpNameService::Query()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::Query(). Return ER_OK\n");
        return ER_OK;
    }
    ASSERT_STATE("Query");
    printf("Exit IpNameService::Query(). Return m_pimpl->Query(***)\n");
    return m_pimpl->Query(transportMask, mdnsPacket);
}

QStatus IpNameService::Response(TransportMask transportMask, uint32_t ttl, MDNSPacket mdnsPacket)
{
    printf("Enter IpNameService::Response()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::Response(). Return ER_OK\n");
        return ER_OK;
    }
    ASSERT_STATE("Response");
    printf("Exit IpNameService::Response(). Return m_pimpl->Response(***)\n");
    return m_pimpl->Response(transportMask, ttl, mdnsPacket);
}

bool IpNameService::RemoveFromPeerInfoMap(const qcc::String& guid)
{
    printf("Enter IpNameService::RemoveFromPeerInfoMap()\n");
    if (m_destroyed) {
        printf("Exit IpNameService::RemoveFromPeerInfoMap(). Return false\n");
        return false;
    }
    ASSERT_STATE("RemoveFromPeerInfoMap");
    printf("Exit IpNameService::RemoveFromPeerInfoMap(). Return m_pimpl ->RemoveFromPeerInfoMap(guid)\n");
    return m_pimpl->RemoveFromPeerInfoMap(guid);
}

} // namespace ajn
