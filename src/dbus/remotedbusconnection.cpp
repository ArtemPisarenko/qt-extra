/**
 * @file    remotedbusconnection.cpp
 * @author  Artem Pisarenko
 * @date    28.03.2018
 * @brief   Implementation of RemoteDBusConnection class
 */

#include <qglobal.h>
#ifdef Q_OS_LINUX
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif
#include <qmutex.h>
#include <qsemaphore.h>
#include <qdbuserror.h>
#include <qtcpsocket.h>
#include <qtcpserver.h>
#include <qtimer.h>
#include "../core/qt.h"
#include "remotedbusconnection.h"

namespace QtExtra {

class RemoteDBusConnectionTunnel : public QObject
{
    Q_OBJECT
public:
#ifdef Q_OS_LINUX
    struct KeepaliveParams {
        bool active;
        int keepcnt, keepidle, keepintvl;
    };
#endif

    RemoteDBusConnectionTunnel();
    ~RemoteDBusConnectionTunnel();

public Q_SLOTS:
    void openChannel(const QString &remote_hostname, quint16 remote_port, QAbstractSocket::NetworkLayerProtocol remote_protocol);
    void closeChannel();
    void abortChannel();
    void setRemoteSocketOption(QAbstractSocket::SocketOption option, const QVariant &value);
#ifdef Q_OS_LINUX
    void applyRemoteSocketKeepaliveParams();
#endif
    void startConnectionTimer();
    void processRemoteSocketConnected();
    void processRemoteSocketDisconnected();
    void processRemoteSocketError();
    void processConnectionTimeout();
    void processConnectionFailure(bool socket_error);
    void disconnectRemoteSocket(bool graceful);
    bool startLocalServer();
    void stopLocalServer();
    void processLocalServerNewConnection();
    void processRemoteSocketReadyRead();
    void processLocalSocketReadyRead();
    void transferDataFromSocketToSocket(QTcpSocket *src_socket, QTcpSocket *dest_socket);
    void syncStartWrappedOperation();
    void asyncStopWrappedOperation();
    bool isWrappedOperationTimedOut();
    void processWrappedOperationTimeout();

Q_SIGNALS:
    void channelOpened(bool success, quint16 local_port = 0);
    void channelError(const QString &message);
    void channelClosed(bool success);

public:
    QMutex mutex;
    int connection_timeout_ms, wrapped_operation_timeout_ms;
    QSemaphore wrapped_operation_semaphore;
    bool wrapped_operation_timed_out;
    QTcpSocket remote_socket;
    QTcpSocket *local_socket;
    QTcpServer local_server;
    QTimer connection_timer, wrapped_operation_timer;
#ifdef Q_OS_LINUX
    KeepaliveParams keepalive_params;
#endif
};

RemoteDBusConnectionTunnel::RemoteDBusConnectionTunnel() :
    QObject(0),
    connection_timeout_ms(-1), wrapped_operation_timeout_ms(-1),
    wrapped_operation_semaphore(1),
    wrapped_operation_timed_out(false),
    remote_socket(this),
    local_socket(nullptr), local_server(this),
    connection_timer(this), wrapped_operation_timer(this)
{
    local_server.setMaxPendingConnections(1);

    QObject::connect(&remote_socket, &QTcpSocket::connected,
                     this, &RemoteDBusConnectionTunnel::processRemoteSocketConnected);
    QObject::connect(&remote_socket, &QTcpSocket::disconnected,
                     this, &RemoteDBusConnectionTunnel::processRemoteSocketDisconnected);
    QObject::connect(&remote_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                     this, &RemoteDBusConnectionTunnel::processRemoteSocketError);
    QObject::connect(&remote_socket, &QTcpSocket::readyRead,
                     this, &RemoteDBusConnectionTunnel::processRemoteSocketReadyRead);

    QObject::connect(&local_server, &QTcpServer::newConnection,
                     this, &RemoteDBusConnectionTunnel::processLocalServerNewConnection);

    connection_timer.setSingleShot(true);
    QObject::connect(&connection_timer, &QTimer::timeout,
                     this, &RemoteDBusConnectionTunnel::processConnectionTimeout);

    wrapped_operation_timer.setSingleShot(true);
    QObject::connect(&wrapped_operation_timer, &QTimer::timeout,
                     this, &RemoteDBusConnectionTunnel::processWrappedOperationTimeout);

#ifdef Q_OS_LINUX
    keepalive_params.active = false;
#endif
}

RemoteDBusConnectionTunnel::~RemoteDBusConnectionTunnel()
{
    abortChannel();
}

void RemoteDBusConnectionTunnel::openChannel(const QString &remote_hostname, quint16 remote_port, QAbstractSocket::NetworkLayerProtocol remote_protocol)
{
    if (remote_socket.state() != QAbstractSocket::UnconnectedState)
        Q_EMIT channelOpened(false);

    if (!startLocalServer()) {
        Q_EMIT channelError("Internal error: failed to start local server");
        Q_EMIT channelOpened(false);
    }

    remote_socket.connectToHost(remote_hostname, remote_port, QIODevice::ReadWrite, remote_protocol);

    if (remote_socket.state() != QAbstractSocket::ConnectedState)
        startConnectionTimer();
}

void RemoteDBusConnectionTunnel::closeChannel()
{
    switch (remote_socket.state()) {
    case QAbstractSocket::UnconnectedState:
        Q_EMIT channelClosed(false);
        break;
    case QAbstractSocket::HostLookupState:
    case QAbstractSocket::ConnectingState:
        disconnectRemoteSocket(false);
        stopLocalServer();
        Q_EMIT channelClosed(true);
        break;
    case QAbstractSocket::ConnectedState:
        disconnectRemoteSocket(true);
        break;
    case QAbstractSocket::ClosingState:
        break;
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
        break;
    }
}

void RemoteDBusConnectionTunnel::abortChannel()
{
    switch (remote_socket.state()) {
    case QAbstractSocket::UnconnectedState:
    case QAbstractSocket::ClosingState:
        break;
    case QAbstractSocket::HostLookupState:
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::ConnectedState:
        disconnectRemoteSocket(false);
        stopLocalServer();
        break;
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
        break;
    }
}

void RemoteDBusConnectionTunnel::setRemoteSocketOption(QAbstractSocket::SocketOption option, const QVariant &value)
{
    remote_socket.setSocketOption(option, value);
}

#ifdef Q_OS_LINUX
void RemoteDBusConnectionTunnel::applyRemoteSocketKeepaliveParams()
{
    QMutexLocker locker(&mutex);
    if (!keepalive_params.active)
        return;
    qintptr sd = remote_socket.socketDescriptor();
    if (sd == -1)
        return;
    int optval;
    socklen_t optlen = sizeof(optval);
    bool success = true;
    if (success) {
        optval = keepalive_params.keepcnt;
        success &= (setsockopt(sd, SOL_TCP, TCP_KEEPCNT, &optval, optlen) == 0);
    }
    if (success) {
        optval = keepalive_params.keepidle;
        success &= (setsockopt(sd, SOL_SOCKET, TCP_KEEPIDLE, &optval, optlen) == 0);
    }
    if (success) {
        optval = keepalive_params.keepintvl;
        success &= (setsockopt(sd, SOL_TCP, TCP_KEEPINTVL, &optval, optlen) == 0);
    }
    if (!success) {
        char error_buf[255];
        Q_EMIT channelError(QString("Failed to set keepalive options for remote socket with error: %1 (%2)")
                            .arg(errno).arg(strerror_r(errno, error_buf, sizeof(error_buf))));
    }
}
#endif

void RemoteDBusConnectionTunnel::startConnectionTimer()
{
    QMutexLocker locker(&mutex);
    if (connection_timeout_ms != -1)
        connection_timer.start(connection_timeout_ms);
}

void RemoteDBusConnectionTunnel::processRemoteSocketConnected()
{
    connection_timer.stop();
#ifdef Q_OS_LINUX
    applyRemoteSocketKeepaliveParams();
#endif

    Q_ASSERT(local_server.isListening());
    Q_EMIT channelOpened(true, local_server.serverPort());
}

void RemoteDBusConnectionTunnel::processRemoteSocketDisconnected()
{
    connection_timer.stop();

    stopLocalServer();
    Q_EMIT channelClosed(true);
}

void RemoteDBusConnectionTunnel::processRemoteSocketError()
{
    Q_EMIT channelError(QString("Remote connection error: %1").arg(remote_socket.errorString()));
    processConnectionFailure(true);
}

void RemoteDBusConnectionTunnel::processConnectionTimeout()
{
    processConnectionFailure(false);
}

void RemoteDBusConnectionTunnel::processConnectionFailure(bool socket_error)
{
    switch (remote_socket.state()) {
    case QAbstractSocket::HostLookupState:
    case QAbstractSocket::ConnectingState:
        disconnectRemoteSocket(false);
        stopLocalServer();
        if (!socket_error)
            Q_EMIT channelError("Remote connect attempt timed out");
        Q_EMIT channelOpened(false);
        break;
    case QAbstractSocket::ClosingState:
        disconnectRemoteSocket(false);
        stopLocalServer();
        if (!socket_error)
            Q_EMIT channelError("Remote disconnect attempt timed out, aborting");
        Q_EMIT channelClosed(true);
        break;
    case QAbstractSocket::UnconnectedState:
    case QAbstractSocket::ConnectedState:
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
        Q_ASSERT(false);
        break;
    }
}

void RemoteDBusConnectionTunnel::disconnectRemoteSocket(bool graceful)
{
    connection_timer.stop();
    if (graceful) {
        remote_socket.disconnectFromHost();
        if (remote_socket.state() != QAbstractSocket::UnconnectedState)
            startConnectionTimer();
    } else {
        bool last_b_s_state = remote_socket.blockSignals(true);
        remote_socket.abort();
        remote_socket.blockSignals(last_b_s_state);
    }
}

bool RemoteDBusConnectionTunnel::startLocalServer()
{
    bool result = local_server.listen(QHostAddress::LocalHost);
    if (result)
        local_server.resumeAccepting();
    return result;
}

void RemoteDBusConnectionTunnel::stopLocalServer()
{
    if (local_server.isListening())
        local_server.close();
    if (local_socket) {
        local_socket->blockSignals(true);
        local_socket->abort();
        local_socket->deleteLater();
        local_socket = nullptr;
    }
}

void RemoteDBusConnectionTunnel::processLocalServerNewConnection()
{
    local_server.pauseAccepting();
    Q_ASSERT(local_socket == nullptr);
    local_socket = local_server.nextPendingConnection();
    QObject::connect(local_socket, &QTcpSocket::readyRead,
                     this, &RemoteDBusConnectionTunnel::processLocalSocketReadyRead);
}

void RemoteDBusConnectionTunnel::processRemoteSocketReadyRead()
{
    if (local_socket == nullptr)
        return;
    transferDataFromSocketToSocket(&remote_socket, local_socket);
}

void RemoteDBusConnectionTunnel::processLocalSocketReadyRead()
{
    if (local_socket == nullptr)
        return;
    transferDataFromSocketToSocket(local_socket, &remote_socket);
}

void RemoteDBusConnectionTunnel::transferDataFromSocketToSocket(QTcpSocket *src_socket, QTcpSocket *dest_socket)
{
    QByteArray data = src_socket->readAll();
    if (data.isEmpty())
        return;
    qint64 written Q_DECL_UNUSED;
    written = dest_socket->write(data);
    Q_ASSERT((written < 0) || (written == data.size()));
}

void RemoteDBusConnectionTunnel::syncStartWrappedOperation()
{
    wrapped_operation_timed_out = false;
    wrapped_operation_semaphore.acquire();
    if (wrapped_operation_timeout_ms != -1)
        wrapped_operation_timer.start(wrapped_operation_timeout_ms);
}

void RemoteDBusConnectionTunnel::asyncStopWrappedOperation()
{
    if (wrapped_operation_timeout_ms != -1)
        QTMETAMETHOD_INVOKE_QUEUED(&wrapped_operation_timer, stop);
    wrapped_operation_semaphore.release();
}

bool RemoteDBusConnectionTunnel::isWrappedOperationTimedOut()
{
    return wrapped_operation_timed_out;
}

void RemoteDBusConnectionTunnel::processWrappedOperationTimeout()
{
    wrapped_operation_timed_out = true;
    if (remote_socket.state() != QAbstractSocket::ConnectedState)
        return;
    abortChannel();
    wrapped_operation_semaphore.acquire();
    wrapped_operation_semaphore.release();
    Q_EMIT channelClosed(true);
}

RemoteDBusConnection::RemoteDBusConnection(const QString &name, QObject *parent) :
    QObject(parent),
    ref(nullptr), ref_name(name),
    tunnel_thread(this)
{
    qRegisterMetaType<QAbstractSocket::NetworkLayerProtocol>();

    tunnel = new RemoteDBusConnectionTunnel();
    Q_CHECK_PTR(tunnel);
    QObject::connect(tunnel, &RemoteDBusConnectionTunnel::channelOpened,
                     this, &RemoteDBusConnection::processTunnelChannelOpened);
    QObject::connect(tunnel, &RemoteDBusConnectionTunnel::channelClosed,
                     this, &RemoteDBusConnection::processTunnelChannelClosed);
    QObject::connect(tunnel, &RemoteDBusConnectionTunnel::channelError,
                     this, &RemoteDBusConnection::connectionError);

    tunnel->moveToThread(&tunnel_thread);
    QObject::connect(&tunnel_thread, &QThread::finished,
                     tunnel, &RemoteDBusConnectionTunnel::deleteLater);
    tunnel_thread.start();
}

RemoteDBusConnection::~RemoteDBusConnection()
{
    if (isConnectionOpened()) {
        dropNativeDBusConnection();
        QTMETAMETHOD_INVOKE_QUEUED(tunnel, abortChannel);
    }
    tunnel_thread.quit();
    tunnel_thread.wait();
    Q_ASSERT(tunnel_thread.isFinished());
}

void RemoteDBusConnection::setConnectionTimeout(int timeout_ms)
{
    QMutexLocker locker(&tunnel->mutex);
    tunnel->connection_timeout_ms = timeout_ms;
}

void RemoteDBusConnection::setWrappedOperationTimeout(int timeout_ms)
{
    tunnel->wrapped_operation_timeout_ms = timeout_ms;
}

bool RemoteDBusConnection::setKeepaliveEnabled(bool enabled)
{
#ifdef Q_OS_WIN
    if (isConnectionOpened())
        return false;
#endif
    QTMETAMETHOD_INVOKE_ARGS2(tunnel, setRemoteSocketOption, Qt::BlockingQueuedConnection,
                                     (QAbstractSocket::SocketOption, QAbstractSocket::KeepAliveOption),
                                     (const QVariant&, QVariant((int)enabled)));
    return true;
}

void RemoteDBusConnection::setLowDelayOption(bool enabled)
{
    QTMETAMETHOD_INVOKE_ARGS2(tunnel, setRemoteSocketOption, Qt::BlockingQueuedConnection,
                                     (QAbstractSocket::SocketOption, QAbstractSocket::LowDelayOption),
                                     (const QVariant&, QVariant((int)enabled)));
}

#ifdef Q_OS_LINUX
void RemoteDBusConnection::setKeepaliveParameters(int keepcnt, int keepidle, int keepintvl)
{
    tunnel->mutex.lock();
    tunnel->keepalive_params.active = true;
    tunnel->keepalive_params.keepcnt = keepcnt;
    tunnel->keepalive_params.keepidle = keepidle;
    tunnel->keepalive_params.keepintvl = keepintvl;
    tunnel->mutex.unlock();
    if (isConnectionOpened())
        QTMETAMETHOD_INVOKE(tunnel, applyRemoteSocketKeepaliveParams, Qt::BlockingQueuedConnection);
}

void RemoteDBusConnection::unsetKeepaliveParameters()
{
    tunnel->mutex.lock();
    tunnel->keepalive_params.active = false;
    tunnel->mutex.unlock();
}
#endif

bool RemoteDBusConnection::isConnectionOpened() const
{
    return (ref != nullptr);
}

bool RemoteDBusConnection::openConnection(const QString &hostname, quint16 port, QAbstractSocket::NetworkLayerProtocol protocol)
{
    if (isConnectionOpened())
        return false;
    QTMETAMETHOD_INVOKE_QUEUED_ARGS3(tunnel, openChannel, (const QString&, hostname), (quint16, port), (QAbstractSocket::NetworkLayerProtocol, protocol));
    return true;
}

bool RemoteDBusConnection::closeConnection()
{
    if (!isConnectionOpened())
        return false;
    dropNativeDBusConnection();
    QTMETAMETHOD_INVOKE_QUEUED(tunnel, closeChannel);
    return true;
}

QDBusError RemoteDBusConnection::lastError() const
{
    if (!isConnectionOpened())
        return QDBusError();
    return ref->lastError();
}

bool RemoteDBusConnection::send(const QDBusMessage &message)
{
    return executeWrappedOperation([&]() {
        return ref->send(message);
    });
}

bool RemoteDBusConnection::registerObject(const QString &path, QObject *object, QDBusConnection::RegisterOptions options)
{
    return executeWrappedOperation([&]() {
        return ref->registerObject(path, object, options);
    });
}

bool RemoteDBusConnection::registerObject(const QString &path, const QString &interface, QObject *object, QDBusConnection::RegisterOptions options)
{
    return executeWrappedOperation([&]() {
        return ref->registerObject(path, interface, object, options);
    });
}

void RemoteDBusConnection::unregisterObject(const QString &path, QDBusConnection::UnregisterMode mode)
{
    executeWrappedOperation([&]() {
        ref->unregisterObject(path, mode);
        return ref->isConnected();
    });
}

QObject *RemoteDBusConnection::objectRegisteredAt(const QString &path)
{
    QObject *object = nullptr;
    executeWrappedOperation([&]() {
        object = ref->objectRegisteredAt(path);
        return ref->isConnected();
    });
    return object;
}

bool RemoteDBusConnection::registerVirtualObject(const QString &path, QDBusVirtualObject *object, QDBusConnection::VirtualObjectRegisterOption options)
{
    return executeWrappedOperation([&]() {
        return ref->registerVirtualObject(path, object, options);
    });
}

bool RemoteDBusConnection::registerService(const QString &serviceName)
{
    return executeWrappedOperation([&]() {
        return ref->registerService(serviceName);
    });
}

bool RemoteDBusConnection::unregisterService(const QString &serviceName)
{
    return executeWrappedOperation([&]() {
        return ref->unregisterService(serviceName);
    });
}

bool RemoteDBusConnection::constructInterface(std::function<void (const QDBusConnection &)> constructor)
{
    return executeWrappedOperation([&]() {
        constructor(*ref);
        return ref->isConnected();
    });
}

void RemoteDBusConnection::processTunnelChannelOpened(bool success, quint16 local_port)
{
    if (success) {
        QString dbus_address = QString("tcp:host=localhost,port=%1").arg(local_port);
        ref = new QDBusConnection(QDBusConnection::connectToBus(dbus_address, ref_name));
        Q_CHECK_PTR(ref);
        if (ref->isConnected()) {
            Q_EMIT connectionOpened(true);
        } else {
            QDBusError dbus_error = ref->lastError();
            dropNativeDBusConnection();
            QTMETAMETHOD_INVOKE_QUEUED(tunnel, abortChannel);
            Q_EMIT connectionError("D-Bus connection failed with " + formatDBusErrorDetails(&dbus_error));
            Q_EMIT connectionOpened(false);
        }
    } else {
        Q_EMIT connectionOpened(false);
    }
}

void RemoteDBusConnection::processTunnelChannelClosed(bool success)
{
    if (!success)
        return;
    dropNativeDBusConnection();
    Q_EMIT connectionClosed();
}

bool RemoteDBusConnection::executeWrappedOperation(std::function<bool ()> operation)
{
    if (!(isConnectionOpened() && ref->isConnected()))
        return false;
    QTMETAMETHOD_INVOKE(tunnel, syncStartWrappedOperation, Qt::BlockingQueuedConnection);
    bool success = operation();
    if (!success) {
        QString error_message;
        if (!tunnel->isWrappedOperationTimedOut()) {
            QDBusError dbus_error = ref->lastError();
            error_message = "D-Bus operation failed with " + formatDBusErrorDetails(&dbus_error);
        } else {
            error_message = "D-Bus operation timed out";
        }
        QTMETAMETHOD_INVOKE_QUEUED_ARGS1(this, connectionError, (const QString&, error_message));
    }
    tunnel->asyncStopWrappedOperation();
    return success;
}

void RemoteDBusConnection::dropNativeDBusConnection()
{
    if (ref == nullptr)
        return;
    ref->disconnectFromBus(ref_name);
    delete ref;
    ref = nullptr;
}

QString RemoteDBusConnection::formatDBusErrorDetails(const QDBusError *error)
{
    if (error->isValid())
        return QString("error:\nName: %1\nMessage: %2").arg(error->name()).arg(error->message());
    return "no error";
}

} // namespace QtExtra

#include "remotedbusconnection.moc"
