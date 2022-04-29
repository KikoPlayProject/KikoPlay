/**
  @file
  @author Stefan Frings
*/

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QMap>
#include <QString>
#include <QTcpSocket>
#include "httpglobal.h"
#include "httpcookie.h"

namespace stefanfrings {

/**
  This object represents a HTTP response, used to return something to the web client.
  <p>
  <code><pre>
    response.setStatus(200,"OK"); // optional, because this is the default
    response.writeBody("Hello");
    response.writeBody("World!",true);
  </pre></code>
  <p>
  Example how to return an error:
  <code><pre>
    response.setStatus(500,"server error");
    response.write("The request cannot be processed because the servers is broken",true);
  </pre></code>
  <p>
  In case of large responses (e.g. file downloads), a Content-Length header should be set
  before calling write(). Web Browsers use that information to display a progress bar.
*/

class DECLSPEC HttpResponse {
    Q_DISABLE_COPY(HttpResponse)
public:

    /**
      Constructor.
      @param socket used to write the response
    */
    HttpResponse(QTcpSocket *socket);

    enum
    {
        /// Request was successful
        OK = 200,
        /// Request was successful and a resource was created
        Created = 201,
        /// Request was accepted for processing, not completed yet.
        Accepted = 202,
        /// %Range request was successful
        PartialContent = 206,
        /// Resource has moved permanently
        MovedPermanently = 301,
        /// Resource is available at an alternate URI
        Found = 302,
        /// Bad client request
        BadRequest = 400,
        /// Client is unauthorized to access the resource
        Unauthorized = 401,
        /// Access to the resource is forbidden
        Forbidden = 403,
        /// Resource was not found
        NotFound = 404,
        /// Method is not valid for the resource
        MethodNotAllowed = 405,
        /// The request could not be completed due to a conflict with the current state of the resource
        Conflict = 409,
        /// Requested range not satisfiable
        RequestedRangeNotSatisfiable = 416,
        /// An internal server error occurred
        InternalServerError = 500,
        /// Invalid response from server while acting as a gateway
        BadGateway = 502,
        /// %Server unable to handle request due to overload
        ServiceUnavailable = 503,
        /// %Server does not supports the HTTP version in the request
        HttpVersionNotSupported = 505
    };

    /**
      Set a HTTP response header.
      You must call this method before the first write().
      @param name name of the header
      @param value value of the header
    */
    void setHeader(const QByteArray name, const QByteArray value);

    /**
      Set a HTTP response header.
      You must call this method before the first write().
      @param name name of the header
      @param value value of the header
    */
    void setHeader(const QByteArray name, const int value);

    /** Get the map of HTTP response headers */
    QMap<QByteArray,QByteArray>& getHeaders();

    /** Get the map of cookies */
    QMap<QByteArray,HttpCookie>& getCookies();

    /**
      Set status code and description. The default is 200,OK.
      You must call this method before the first write().
    */
    void setStatus(const int statusCode, const QByteArray description=QByteArray());

    /** Return the status code. */
    int getStatusCode() const;

    /**
      Write body data to the socket.
      <p>
      The HTTP status line, headers and cookies are sent automatically before the body.
      <p>
      If the response contains only a single chunk (indicated by lastPart=true),
      then a Content-Length header is automatically set.
      <p>
      Chunked mode is automatically selected if there is no Content-Length header
      and also no Connection:close header.
      @param data Data bytes of the body
      @param lastPart Indicates that this is the last chunk of data and flushes the output buffer.
    */
    void write(const QByteArray data, const bool lastPart=false);

    /**
      Indicates whether the body has been sent completely (write() has been called with lastPart=true).
    */
    bool hasSentLastPart() const;

    /**
      Set a cookie.
      You must call this method before the first write().
    */
    void setCookie(const HttpCookie& cookie);

    /**
      Send a redirect response to the browser.
      Cannot be combined with write().
      @param url Destination URL
    */
    void redirect(const QByteArray& url);

    /**
     * Flush the output buffer (of the underlying socket).
     * You normally don't need to call this method because flush is
     * automatically called after HttpRequestHandler::service() returns.
     */
    void flush();

    /**
     * May be used to check whether the connection to the web client has been lost.
     * This might be useful to cancel the generation of large or slow responses.
     */
    bool isConnected() const;

private:

    /** Request headers */
    QMap<QByteArray,QByteArray> headers;

    /** Socket for writing output */
    QTcpSocket* socket;

    /** HTTP status code*/
    int statusCode;

    /** HTTP status code description */
    QByteArray statusText;

    /** Indicator whether headers have been sent */
    bool sentHeaders;

    /** Indicator whether the body has been sent completely */
    bool sentLastPart;

    /** Whether the response is sent in chunked mode */
    bool chunkedMode;

    /** Cookies */
    QMap<QByteArray,HttpCookie> cookies;

    /** Write raw data to the socket. This method blocks until all bytes have been passed to the TCP buffer */
    bool writeToSocket(QByteArray data);

    /**
      Write the response HTTP status and headers to the socket.
      Calling this method is optional, because writeBody() calls
      it automatically when required.
    */
    void writeHeaders();

};

} // end of namespace

#endif // HTTPRESPONSE_H
