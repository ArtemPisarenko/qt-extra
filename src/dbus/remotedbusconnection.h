/**
 * @file    remotedbusconnection.h
 * @author  Artem Pisarenko
 * @date    28.03.2018
 * @brief   Header and documentation of RemoteDBusConnection class
 */

#ifndef QTEXTRA_REMOTEDBUSCONNECTION_H
#define QTEXTRA_REMOTEDBUSCONNECTION_H

#include <functional>
#include <qobject.h>
#include <qabstractsocket.h>
#include <qdbusconnection.h>
#include <qthread.h>

namespace QtExtra {

//! Remote QDBusConnection wrapper class
/*!
  A wrapper class for using QDBusConnection via TCP/IP transport with remote dbus daemon.

  One may instantiate such D-Bus connection using following code snippet:
  \code{.cpp}
  QDBusConnection::connectToBus("tcp:host=<remote_hostname>,port=<remote_port>", "my_connection_name");
  \endcode
  It works, but D-Bus and its Qt implementation wasn't designed for remote communication (outside of local machine),
  so it doesn't handles possible failures in underlying transport in adequate manner.
  For example, it has timeout for connection attempt of ~25 secs ! It isn't possible to change timeout value
  in any way (at least author didn't found). There are no signals like "connected()" or "disconnected()".
  The worst consequense is that some of QtDBus calls will block calling thread for a large amount of time
  in case if tcp/ip connection will hang.

  This wrapper class addresses the problem and additionally provides some configuration features
  for underlying transport. This class combines QDBusConnection and QTcpSocket in manner matching Qt philosophy.

  A class implements tcp/ip tunnel between localhost and remote side.

  \note
  Class objects require an event loop (i.e. there are no waitFor<...> semantic for using them within any thread context
  in blocking manner).

  Each time connection opened it starts listening on free localhost port, establishes connection with remote port,
  creates QDBusConnection instance connected to localhost port and transfers data between connection endpoints.
  On closing connection it does corresponding disconnect/deallocate/free things in reverse order.
  Since QDBusConnection connect/disconnect calls are blocking, class uses separate thread
  for all networking and timeout detections.
  Internal QDBusConnection instance (reference) lives only for active connection time.
  It isn't intended to be used externally (although, there is one exception, see constructInterface() method).
  Instead class provides subset of QDBusConnection API: it has methods wrapping object's methods and
  other QtDBus usages requiring QDBusConnection object via reference.

  \note
  Not all QtDBus API implemented at now, but it's easily extendable.

  Wrapped operation calls are invoked within this class thread, but they are protected with configurable timeout.
  If timeout expires, then connection automatically dropped, thus unblocking calling thread.
  It still possible to depend on blocking when using other QtDBus classes associated with this class connection.
  For example, by calling QDBusAbstractInterface::call() on proxy object, constructed with this class object instance.

  \note
  There are corner cases possible (race condition) when successfull call eventually timeouts and connection drops.
  Adjust timeouts accordingly. They must be large enough.

  Instantiated QDBusConnection object associates with connection name, provided in class constructor.
  This name must be unique and shouldn't be used in any other D-Bus connection instances within application.
  Otherwise, it broke class behavior.

  \note
  Actually, it may be used, but only before opening connection with this name and after closing it.
  But author don't recommend, since "using" means not only connected state but also,
  that at least one QDBusConnection object exists/allocated with this name.

  Basic usage of class illustrated in following example:
  \code{.cpp}
  ...
  RemoteDBusConnection r_dbus_connection("myconnection");
  r_dbus_connection.setConnectionTimeout(500);
  r_dbus_connection.setWrappedOperationTimeout(100);
  ...
  r_dbus_connection.openConnection("remote.host", 12345);
  ... // wait for connectionOpened() signal
  if (r_dbus_connection.isConnectionOpened()) {
    ...
    MyDBusObjectProxy *proxy; // user class inherited from QDBusAbstractInterface, such as generated from qdbusxml2cpp
    bool success = r_dbus_connection.constructInterface([&](const QDBusConnection &connection) {
      proxy = new MyDBusObjectProxy(..., connection);
    }); // lambda function used to get QDBusConnection reference required to construct user class
    ...
    r_dbus_connection.registerService("myservicename");
    ...
    delete proxy;
    r_dbus_connection.closeConnection();
    ... // wait for connectionClosed() signal
  } else {
    ... // handle failed attempt
  }
  \endcode

  Remote connection transport may be configured, for examlpe, keepalive options are available to be set.
  Using network proxy for connection isn't supported, although wrapped QAbstractSocket socket already implements it.
  The problem is that if proxy authentication required, QAbstractSocket fires proxyAuthenticationRequired() signal,
  which requires synchronous slot connection, where slot must handle request and return response in passed argument.
  It isn't possible to acheive SIMILAR behavior in this class, because its thread is blocked in QDBusConnection::connectToBus()
  call at this moment, and forwarding QAbstractSocket::proxyAuthenticationRequired() signal from network thread
  using blocked queued connection ends in deadlock.
  Yes, author COULD add some limited support, for example:
   - signalling authentication request in thread other than object used in (main thread), forbiding called slot to access/lock on main thread
     (too severe limitation, unsolvable in majority of applications),
   - or without authentication (most likely),
  Maybe in some future it will be done.

*/
class RemoteDBusConnectionTunnel;

class RemoteDBusConnection : public QObject
{
    Q_OBJECT
public:

    //! Constructs an object instance.
    /*!
      \param name unique QDBusConnection name for this instance
             (no other D-Bus connections instantces with this name allowed while isConnectionOpened())
      \sa ~RemoteDBusConnection() and isConnectionOpened()
    */
    explicit RemoteDBusConnection(const QString &name, QObject *parent = nullptr);

    //! Destructs an object instance.
    /*! Closes opened connection, if any (remote connection being aborted, no signals emitted).
      \sa ~RemoteDBusConnection() and isConnectionOpened()
    */
    ~RemoteDBusConnection();

    //! Sets timeout value used in remote connect/disconnect operations.
    /*!
      New value applies to next operations.
      \param timeout_ms value in milliseconds
      \sa openConnection() and closeConnection()
    */
    void setConnectionTimeout(int timeout_ms);

    //! Sets timeout value used in interface methods wrapping blocking operations.
    /*!
      New value applies to next operations.
      If called wrapped operation timeouts, then connection is dropped (signal will be emitted in next event loop cycle),
      and corresponding call returns error value (depends on method interface).
      \param timeout_ms value in milliseconds
    */
    void setWrappedOperationTimeout(int timeout_ms);

    //! Sets keepalive option for remote connection socket.
    /*!
      Option can be set at any time, but may be platform-specific effects.
      \return true on success, false otherwise
      \sa setKeepaliveParameters(), QAbstractSocket::KeepAliveOption and QAbstractSocket::setSocketOption()
    */
    bool setKeepaliveEnabled(bool enabled);

    //! Sets low delay option (TCP_NODELAY) for remote connection socket.
    /*!
      Option can be set at any time, but may be platform-specific effects.
      \sa QAbstractSocket::LowDelayOption and QAbstractSocket::setSocketOption()
    */
    void setLowDelayOption(bool enabled);

#ifdef Q_OS_LINUX
    //! Sets keepalive parameters for remote connection socket.
    /*!
      Can be set at any time.
      Available only for Linux platform.
      See http://www.tldp.org/HOWTO/html_single/TCP-Keepalive-HOWTO/ for details.
      \param keepcnt corresponds to TCP_KEEPCNT
      \param keepidle corresponds to TCP_KEEPIDLE
      \param keepintvl corresponds to TCP_KEEPINTVL
      \sa setKeepaliveEnabled() and unsetKeepaliveParameters()
    */
    void setKeepaliveParameters(int keepcnt, int keepidle, int keepintvl);

    //! Unsets keepalive parameters for remote connection socket.
    /*!
      Unsets parameters previously being set.
      Changes will be applied at next connection.
      \sa setKeepaliveParameters()
    */
    void unsetKeepaliveParameters();
#endif

    //! Check if connection is open
    /*!
      Wrapped interfaces methods available only when this method returns true.
      \return current connection state (true - opened)
      \sa RemoteDBusConnection(), openConnection(), closeConnection(), connectionOpened(), connectionClosed()
    */
    bool isConnectionOpened() const;

    //! Starts opening connection
    /*!
      Initiates establishment of connection with remote dbus daemon, using timeout previously set.
      Parameters matches those defined in QAbstractSocket::connectToHost().
      \return true - if started successfully (connectionOpened() signal follows)
              false - failed (already opened)
      \sa isConnectionOpened(), connectionOpened(), closeConnection() and setConnectionTimeout()
    */
    bool openConnection(const QString &hostname, quint16 port,
                        QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::AnyIPProtocol);

    //! Starts closing connection
    /*!
      Initiates graceful disconnection from remote dbus daemon, using timeout previously set.
      \return true - if started successfully (connectionClosed() signal follows)
              false - failed (already closed)
      \sa isConnectionOpened(), connectionClosed(), openConnection() and setConnectionTimeout()
    */
    bool closeConnection();

//@{
   //! Wrapped QDBusConnection object interface
   /*!
     Protected internally with timeout, previously set.
     See equally named QDBusConnection methods.
     If any error occurs, method returns relevant false/null/etc.
     \sa setWrappedOperationTimeout()
    */
    QDBusError lastError() const;
    inline QString name() const {return ref_name;}
    bool send(const QDBusMessage &message);
    bool registerObject(const QString &path, QObject *object,
                        QDBusConnection::RegisterOptions options = QDBusConnection::ExportAdaptors);
    bool registerObject(const QString &path, const QString &interface, QObject *object,
                        QDBusConnection::RegisterOptions options = QDBusConnection::ExportAdaptors);
    void unregisterObject(const QString &path, QDBusConnection::UnregisterMode mode = QDBusConnection::UnregisterNode);
    QObject *objectRegisteredAt(const QString &path);
    bool registerVirtualObject(const QString &path, QDBusVirtualObject *object,
                          QDBusConnection::VirtualObjectRegisterOption options = QDBusConnection::SingleNode);
    bool registerService(const QString &serviceName);
    bool unregisterService(const QString &serviceName);
//@}

    //! Wrapper for constructing QDBusAbstractInterface derived object (proxy)
    /*!
      Protected internally with timeout, previously set.
      Passed constructor functor is being called during this method call.
      Restrictions for functor implementation (violations results in undefined behavior):
      - it shouldn't use passed QDBusConnection reference for any other purpose
      except just for constructing proxy object.
      - callbacks to this class methods are not allowed (to prevent possible deadloacks).
      \param constructor pointer to function calling object constructor
      \return true - success
              false - any error occured, caused QDBusConnection::isConnected() to be false
      \sa setWrappedOperationTimeout()
    */
    bool constructInterface(std::function<void (const QDBusConnection &)> constructor);

Q_SIGNALS:
/*! \defgroup Signals */
/**@{*/

    //! Signals complete procedure of opening connection
    /*!
      Signal is guaranteed to fire after successfull openConnection() call.
      If any error encountered during remote connection attempt, they are signalled with another signal.
      \param success true - connection succesfully opened
                     false - connection attempt failed
      \sa isConnectionOpened(), openConnection(), connectionError(), setConnectionTimeout()
    */
    void connectionOpened(bool success);

    //! Signals error
    /*!
      Any error encountered during opening and closing connection and during connection lifetime is signalled
      here.
      Signal just informs and neither does nothing with connection, nor implicates it's current state.
      It's intended to be used to log or display user message, but not for decisions regarding following actions.
      Usually it followed by corresponding signal
      (for example, connectionOpened(false) if connecion initiated by successfull openConnection() call timed out).
      See class sources for possible error conditions and messages.
      \param message text describing error source
      \sa isConnectionOpened(), openConnection(), closeConnection()
    */
    void connectionError(const QString &message);

    //! Signals complete procedure of closing connection
    /*!
      Signal is guaranteed to fire after successfull closeConnection() call.
      If any error encountered during remote disconnecion attempt, they are signalled with another signal.
      If initiated by closeConnection() call, connection will be closed anyway after timeout expired.
      Also signal also being fired after losing remote connection for any reason.
      \sa isConnectionOpened(), closeConnection(), connectionError(), setConnectionTimeout()
    */
    void connectionClosed();

/**@}*/

//@{
/*! Private definitions */
private Q_SLOTS:
    void processTunnelChannelOpened(bool success, quint16 local_port);
    void processTunnelChannelClosed(bool success);
    bool executeWrappedOperation(std::function<bool ()> operation);
    void dropNativeDBusConnection();
    QString formatDBusErrorDetails(const QDBusError *error);
private:
    QDBusConnection *ref;
    QString ref_name;
    RemoteDBusConnectionTunnel *tunnel;
    QThread tunnel_thread;
//@}
};

} // namespace QtExtra

#endif // QTEXTRA_REMOTEDBUSCONNECTION_H
