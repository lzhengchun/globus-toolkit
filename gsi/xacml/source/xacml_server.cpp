/*
 * Copyright 1999-2006 University of Chicago
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "soapH.h"

#include "xacml_server.h"
#include "xacml_i.h"

#include <limits.h>
#include <unistd.h>
#include <dlfcn.h>

#include <cassert>
#include <sstream>
#include <iomanip>

#ifndef _POSIX_HOST_NAME_MAX
#   define _POSIX_HOST_NAME_MAX 255
#endif

extern "C"
int
xacml_i_accept(
    struct soap                        *soap,
    int                                 socket,
    struct sockaddr                    *addr,
    int                                *n);

#ifndef DONT_DOCUMENT_INTERNAL
namespace xacml
{
void *
service_thread(void * arg)
{
    xacml_server_t server = (xacml_server_t) arg;
    struct sockaddr addr;
    socklen_t namelen;
    struct soap soap;
    char serv[6];

    pthread_mutex_lock(&server->lock);

    soap_init(&soap);
    soap.send_timeout = 10; 
    soap.recv_timeout = 10;
    soap.accept_timeout = 2; 
    soap.bind_flags = SO_REUSEADDR;
    soap.user = server;

    if (server->accept_func != NULL)
    {
        /* Use custom I/O handler wrappers */
        soap.user = server;
        soap.faccept = xacml_i_accept;
        soap.fopen = xacml_i_connect;
        soap.fsend = xacml_i_send;
        soap.frecv = xacml_i_recv;
        soap.fclose = xacml_i_close;
    }

    server->listener = soap_bind(&soap, NULL, server->port, 100);
    if (server->listener < 0)
    {
        server->started = false;
        goto out;
    }
    namelen = sizeof(&addr);
    getsockname(soap.master, &addr, &namelen);
    getnameinfo(&addr, namelen, NULL, 0, serv, sizeof(serv), NI_NUMERICSERV);

    sscanf(serv, "%hu", &server->port);

    while (! server->stopped) 
    {
        pthread_mutex_unlock(&server->lock);

        int s;

        soap.user = server;
        s = soap_accept(&soap);
        if (s < 0)
            continue;
        if (server->request == NULL)
        {
            xacml_request_init(&server->request);
            server->request->server = server;
            soap.user = server->request;
        }
        soap_serve(&soap);
        soap_destroy(&soap);
        soap_end(&soap);

        xacml_request_destroy(server->request);
        server->request = NULL;

        pthread_mutex_lock(&server->lock);
    }
out:
    server->request = NULL;
    soap.user = NULL;
    server->started = false;
    soap_done(&soap);
    pthread_cond_signal(&server->cond);
    pthread_mutex_unlock(&server->lock);

    return NULL;
}

void
extract_attribute_value(
    class xsd__anyType *                attribute,
    std::string &                       value)
{
    value = attribute->__item;
}

int
parse_xacml_query(
    const struct XACMLsamlp__XACMLAuthzDecisionQueryType *
                                        query,
    xacml_request_t                     request)
{
    XACMLcontext__RequestType *         req = query->XACMLcontext__Request;

    xacml_request_set_subject(
            request,
            query->saml__Issuer->__item.c_str());

    for (std::vector<class XACMLcontext__SubjectType * >::iterator i =
                req->XACMLcontext__Subject.begin();
        i != req->XACMLcontext__Subject.end();
        i++)
    {
        if (*i == NULL)
        {
            continue;
        }

        for (std::vector<class XACMLcontext__AttributeType * >::iterator j =
            (*i)->XACMLcontext__Attribute.begin();
            j != (*i)->XACMLcontext__Attribute.end();
            j++)
        {
            for (std::vector<class XACMLcontext__AttributeValueType *>::iterator k =
                        (*j)->XACMLcontext__AttributeValue.begin();
                 k != (*j)->XACMLcontext__AttributeValue.end();
                 k++)
            {
                std::string aval;

                extract_attribute_value(*k, aval);

                xacml_request_add_subject_attribute(
                    request,
                    (*i)->SubjectCategory.c_str(),
                    (*j)->AttributeId.c_str(),
                    (*j)->DataType.c_str(),
                    (*j)->Issuer ? (*j)->Issuer->c_str() : NULL,
                    aval.c_str());
            }
        }
    }

    for (std::vector<class XACMLcontext__ResourceType * >::iterator i =
                req->XACMLcontext__Resource.begin();
        i != req->XACMLcontext__Resource.end();
        i++)
    {
        size_t attribute_count = 0;

        for (std::vector<class XACMLcontext__AttributeType * >::iterator j =
                (*i)->XACMLcontext__Attribute.begin();
            j != (*i)->XACMLcontext__Attribute.end();
            j++)
        {
            for (std::vector<class XACMLcontext__AttributeValueType *>::iterator k =
                        (*j)->XACMLcontext__AttributeValue.begin();
                 k != (*j)->XACMLcontext__AttributeValue.end();
                 k++)
            {
                attribute_count++;
            }
        }

        const char *attribute_id[attribute_count+1];
        const char *data_type[attribute_count+1];
        const char *issuer[attribute_count+1];
        const char *value[attribute_count+1];
        std::string values[attribute_count+1];

        size_t ind = 0;
        for (std::vector<class XACMLcontext__AttributeType * >::iterator j =
                (*i)->XACMLcontext__Attribute.begin();
            j != (*i)->XACMLcontext__Attribute.end();
            j++)
        {
            for (std::vector<class XACMLcontext__AttributeValueType *>::iterator k =
                        (*j)->XACMLcontext__AttributeValue.begin();
                 k != (*j)->XACMLcontext__AttributeValue.end();
                 k++)
            {
                attribute_id[ind] = (*j)->AttributeId.c_str();
                data_type[ind] = (*j)->DataType.c_str();
                issuer[ind] = (*j)->Issuer ? (*j)->Issuer->c_str() : NULL;
                extract_attribute_value(*k, values[ind]);
                value[ind] = values[ind].c_str();
                ind++;
            }
        }
        attribute_id[ind] = NULL;
        data_type[ind] = NULL;
        issuer[ind] = NULL;
        value[ind] = NULL;
        xacml_request_add_resource_attributes(
                request,
                attribute_id,
                data_type,
                issuer,
                value);
    }
    for (std::vector<class XACMLcontext__AttributeType *>::iterator i =
                req->XACMLcontext__Action->XACMLcontext__Attribute.begin();
         i != req->XACMLcontext__Action->XACMLcontext__Attribute.end();
         i++)
    {
        for (std::vector<class XACMLcontext__AttributeValueType *>::iterator j =
                    (*i)->XACMLcontext__AttributeValue.begin();
             j != (*i)->XACMLcontext__AttributeValue.end();
             j++)
        {
            std::string action;

            extract_attribute_value(*j, action);

            xacml_request_add_action_attribute(
                request,
                (*i)->AttributeId.c_str(),
                (*i)->DataType.c_str(),
                (*i)->Issuer ? (*i)->Issuer->c_str() : NULL,
                action.c_str());
        }
    }
    for (std::vector<class XACMLcontext__AttributeType *>::iterator i =
                req->XACMLcontext__Environment->XACMLcontext__Attribute.begin();
         i != req->XACMLcontext__Environment->XACMLcontext__Attribute.end();
         i++)
    {
        for (std::vector<class XACMLcontext__AttributeValueType *>::iterator j =
                    (*i)->XACMLcontext__AttributeValue.begin();
             j != (*i)->XACMLcontext__AttributeValue.end();
             j++)
        {
            std::string envval;

            extract_attribute_value(*j, envval);

            xacml_request_add_environment_attribute(
                request,
                (*i)->AttributeId.c_str(),
                (*i)->DataType.c_str(),
                (*i)->Issuer ? (*i)->Issuer->c_str() : NULL,
                envval.c_str());
        }
    }

    return 0;
}

int
prepare_response(
    xacml_response_t                    response,
    struct samlp__ResponseType *        samlp__Response)
{
    std::ostringstream                  os;

    samlp__Response->saml__Issuer = new saml__NameIDType();
    samlp__Response->saml__Issuer->Format =
            new std::string("urn:oasis:names:tc:SAML:2.0:nameid-format:entity");
    samlp__Response->saml__Issuer->__item = "XACMLService";

    samlp__Response->samlp__Status = new samlp__StatusType();
    samlp__Response->samlp__Status->samlp__StatusCode = new samlp__StatusCodeType();
    samlp__Response->samlp__Status->samlp__StatusCode->Value =
            saml_status_code_strings[response->saml_status_code];

    os << "ID-" << rand();

    samlp__Response->ID = os.str();;

    samlp__Response->Version = "2.0";

    samlp__Response->IssueInstant = time(NULL);
    samlp__Response->__size_32 = 1;
    samlp__Response->__union_32 = new __samlp__union_32();
    
    samlp__Response->__union_32->__union_32 =
            SOAP_UNION__samlp__union_32_saml__Assertion;
    samlp__Response->__union_32->union_32.saml__Assertion =
            new saml__AssertionType();

    samlp__Response->__union_32->union_32.saml__Assertion->IssueInstant =
            time(NULL);

    samlp__Response->__union_32->union_32.saml__Assertion->saml__Issuer =
            new saml__NameIDType();
    samlp__Response->__union_32->union_32.saml__Assertion->saml__Issuer->Format
            =
            new std::string("urn:oasis:names:tc:SAML:2.0:nameid-format:entity");

    const char * issuer;

    if (xacml_response_get_issuer(response, &issuer) != 0)
    {
        return SOAP_SVR_FAULT;
    }
    samlp__Response->__union_32->union_32.saml__Assertion->__item = 
            new char[strlen(issuer)+1];
    strcpy(samlp__Response->__union_32->union_32.saml__Assertion->__item,
           issuer);

    saml__AssertionType * response_assertion =
            samlp__Response->__union_32->union_32.saml__Assertion;

    response_assertion->__size_1 = 1;
    response_assertion->__union_1 = new __saml__union_1();

    response_assertion->__union_1->__union_1 =
            SOAP_UNION__saml__union_1_saml__Statement;
    response_assertion->__union_1->union_1.saml__Statement =
            new XACMLassertion__XACMLAuthzDecisionStatementType();

    XACMLassertion__XACMLAuthzDecisionStatementType * xacml_decision =
            dynamic_cast<XACMLassertion__XACMLAuthzDecisionStatementType *>
                (response_assertion->__union_1->union_1.saml__Statement);

    xacml_decision->XACMLcontext__Response =
            new XACMLcontext__ResponseType();

    XACMLcontext__ResultType * result = new XACMLcontext__ResultType();
    xacml_decision->XACMLcontext__Response->XACMLcontext__Result.push_back(result);

    switch (response->decision)
    {
        case XACML_DECISION_Permit:
            result->XACMLcontext__Decision = XACMLcontext__DecisionType__Permit;
            break;
        case XACML_DECISION_Deny:
            result->XACMLcontext__Decision = XACMLcontext__DecisionType__Deny;
            break;
        case XACML_DECISION_Indeterminate:
        case XACML_DECISION_NotApplicable:
            return SOAP_SVR_FAULT;
    }

    result->XACMLcontext__Status = new XACMLcontext__StatusType();

    result->XACMLcontext__Status->XACMLcontext__StatusCode = 
            new XACMLcontext__StatusCodeType();

    result->XACMLcontext__Status->XACMLcontext__StatusCode->Value =
            xacml_status_code_strings[response->xacml_status_code];

    if (response->obligations.size() != 0)
    {
        result->XACMLpolicy__Obligations =
                new XACMLpolicy__ObligationsType();

        for (xacml::obligations::iterator i = response->obligations.begin();
             i != response->obligations.end();
             i++)
        {
            XACMLpolicy__ObligationType * obligation = new XACMLpolicy__ObligationType();

            result->XACMLpolicy__Obligations->
                    XACMLpolicy__Obligation.push_back(obligation);

            obligation->ObligationId = i->obligation_id;

            switch (i->fulfill_on)
            {
                case XACML_EFFECT_Permit:
                    obligation->FulfillOn = XACMLpolicy__EffectType__Permit;
                    break;
                case XACML_EFFECT_Deny:
                    obligation->FulfillOn = XACMLpolicy__EffectType__Deny;
                    break;
                default:
                    return SOAP_SVR_FAULT;
            }

            for (xacml::attributes::iterator j = i->attributes.begin();
                 j != i->attributes.end();
                 j++)
            {
                XACMLpolicy__AttributeAssignmentType * attr =
                    new XACMLpolicy__AttributeAssignmentType();
                obligation->XACMLpolicy__AttributeAssignment.push_back(attr);

                attr->DataType = j->data_type;
                attr->AttributeId = j->attribute_id;
                attr->__mixed = new char[j->value.length()+1];
                std::strcpy(attr->__mixed, j->value.c_str());
            }
        }
    }
    return SOAP_OK;
}
} // namespace xacml
#endif /* DONT_DOCUMENT_INTERNAL */

/**
 * Initialize XACML server
 * @ingroup xacml_server
 *
 * Create a new XACML authorization server instance. This service, when
 * started with xacml_server_start(), will accept TCP/IP connections, parse an
 * XACML authorization query, and then call the @a handler callback function
 * with the request information. By default, the server will listen for
 * connections on TCP port 8080.
 * 
 * @param server 
 *     Pointer to the service handle to intialize.
 * @param handler
 *     Callback function to perform authorization and create obligations.
 * @param arg
 *     Application-specific argument to @a handler.
 *
 * @retval XACML_RESULT_SUCCESS
 *     Success.
 * @retval XACML_RESULT_INVALID_PARAMETER
 *     Invalid parameter.
 * @see xacml_server_start(), xacml_server_destroy()
 */
int
xacml_server_init(
    xacml_server_t *                    server,
    xacml_authorization_handler_t       handler,
    void *                              arg)
{
    if (server == NULL || handler == NULL)
    {
        return XACML_RESULT_INVALID_PARAMETER;
    }
    (*server) = new xacml_server_s;
    (*server)->port = 8080;
    (*server)->started = false;
    (*server)->stopped = true;
    (*server)->handler = handler;
    (*server)->handler_arg = arg;
    (*server)->io_module = NULL;
    (*server)->accept_func = NULL;
    pthread_mutex_init(&(*server)->lock, NULL);
    pthread_cond_init(&(*server)->cond, NULL);

    return XACML_RESULT_SUCCESS;
}
/* xacml_server_init() */

/**
 * Set the TCP port for a server
 * @ingroup xacml_server
 *
 * Change the TCP port to use for an XACML server. This must be called
 * before the server has been started, or an @a XACML_RESULT_INVALID_STATE
 * error will result.
 *
 * @param server
 *     Server to modify.
 * @param port
 *     TCP port number to use for connections.
 *
 * @retval XACML_RESULT_SUCCESS
 *     Success.
 * @retval XACML_RESULT_INVALID_PARAMETER
 *     Invalid parameter.
 * @retval XACML_RESULT_INVALID_STATE
 *     Invalid state.
 *
 * @see xacml_server_get_port()
 */
int
xacml_server_set_port(
    xacml_server_t                      server,
    unsigned short                      port)
{
    int rc = XACML_RESULT_SUCCESS;

    if (server == NULL)
    {
        rc = XACML_RESULT_INVALID_PARAMETER;
        goto out;
    }

    pthread_mutex_lock(&server->lock);
    if (server->started)
    {
        rc = XACML_RESULT_INVALID_STATE;
    }
    else
    {
        server->port = port;
    }
    pthread_mutex_unlock(&server->lock);

out:
    return rc;
}
/* xacml_server_set_port() */


/**
 * Get the TCP port for a server
 * @ingroup xacml_server
 *
 * Return the TCP port to use for an XACML server. 
 *
 * @param server
 *     Server to query.
 * @param port
 *     Pointer to be set to the TCP port number to use for connections.
 *
 * @retval XACML_RESULT_SUCCESS
 *     Success.
 * @retval XACML_RESULT_INVALID_PARAMETER
 *     Invalid parameter.
 *
 * @see xacml_server_set_port()
 */
int
xacml_server_get_port(
    const xacml_server_t                server,
    unsigned short *                    port)
{
    if (server == NULL || port == NULL)
    {
        return XACML_RESULT_INVALID_PARAMETER;
    }
    pthread_mutex_lock(&server->lock);
    *port = server->port;
    pthread_mutex_unlock(&server->lock);

    return XACML_RESULT_SUCCESS;
}
/* xacml_server_get_port() */

/**
 * Start processing XACML Authorization Queries
 * @ingroup xacml_server
 *
 * Creates a thread to procss XACML authorization queries. This thread
 * will continue to do so until xacml_server_destroy() is called. When
 * a legitimate XACML query has been parsed, the authorization handler
 * function passed to xacml_server_init() will be called with the parsed
 * XACML Authorization query and the application specific argument.
 *
 * @param server
 *     XACML server to start.
 *
 * @retval XACML_RESULT_SUCCESS
 *     Success
 * @retval XACML_RESULT_INVALID_PARAMETER
 *     Invalid parameter
 *
 * @see xacml_server_init(), xacml_server_destroy()
 */
int
xacml_server_start(
    xacml_server_t                      server)
{
    int rc;

    pthread_mutex_lock(&server->lock);
    if (server->started)
    {
        rc = 0;
        goto out;
    }
    rc = pthread_create(&server->service_thread, NULL, xacml::service_thread, server);
    server->started = true;
    server->stopped = false;
out:
    pthread_mutex_unlock(&server->lock);

    return rc;
}
/* xacml_server_start() */

/**
 * Destroy an XACML server
 * @ingroup xacml_server
 *
 * Stop servicing XACML authorization queries, and destroy the server
 * handle. The @a server should not be used after this function returns.
 *
 * @param server
 *     Server to destroy.
 *
 * @return void
 * @see xacml_server_init(), xacml_server_start()
 */
void
xacml_server_destroy(
    xacml_server_t                      server)
{
    void *arg;

    if (server == NULL)
    {
        return;
    }

    pthread_mutex_lock(&server->lock);
    server->stopped = true;
    while (server->started)
    {
        pthread_cond_wait(&server->cond, &server->lock);
    }
    server->started = false;
    server->stopped = true;
    pthread_mutex_unlock(&server->lock);

    pthread_join(server->service_thread, &arg);

    pthread_mutex_destroy(&server->lock);
    pthread_cond_destroy(&server->cond);

    if (server->io_module)
    {
        dlclose(server->io_module);
    }

    delete server;
}


int
xacml_server_set_io_module(
    xacml_server_t                      server,
    const char                         *module)
{
    void                               *mod;
    const xacml_io_descriptor_t        *desc;
    std::string                         module_name;

    module_name = module;
    module_name += ".so";

    mod = dlopen(module_name.c_str(), RTLD_NOW|RTLD_LOCAL);
    desc = reinterpret_cast<xacml_io_descriptor_t *>(dlsym(mod, 
            XACML_IO_DESCRIPTOR));

    server->io_module = mod;
    server->accept_func = desc->accept_func;
    server->connect_func = desc->connect_func;
    server->send_func = desc->send_func;
    server->recv_func = desc->recv_func;
    server->close_func = desc->close_func;

    return XACML_RESULT_SUCCESS;
}
/* xacml_server_set_io_module() */

#ifndef DONT_DOCUMENT_INTERNAL
int
__XACMLService__Authorize(
    struct soap *                       soap,
    struct XACMLsamlp__XACMLAuthzDecisionQueryType *
                                        XACMLsamlp__XACMLAuthzDecisionQuery,
    struct samlp__ResponseType *        samlp__Response)
{
    int                                 rc;
    xacml_server_t                      server;
    xacml_request_t                     request;
    xacml_response_t                    response;

    request = reinterpret_cast<xacml_request_t>(soap->user);
    server = request->server;

    rc = xacml::parse_xacml_query(XACMLsamlp__XACMLAuthzDecisionQuery, request);

    if (rc != 0)
    {
        return SOAP_CLI_FAULT;
    }

    rc = xacml_response_init(&response);
    if (rc != 0)
    {
        return SOAP_SVR_FAULT;
    }

    rc = server->handler(server->handler_arg, request, response);
    if (rc != 0)
    {
        return SOAP_SVR_FAULT;
    }

    rc = xacml::prepare_response(response, samlp__Response);
    if (rc != 0)
    {
        return SOAP_SVR_FAULT;
    }
    xacml_response_destroy(response);

    return SOAP_OK;
}
/* __XACMLService__Authorize() */

extern "C"
int
xacml_i_accept(
    struct soap                        *soap,
    int                                 socket,
    struct sockaddr                    *addr,
    int                                *n)
{
    xacml_server_t                      server = (xacml_server_t) soap->user;
    xacml_request_t                     request;
    socklen_t                           len = *n;
    int                                 rc;
    int                                 sock_out = 0;
    void *                              io_arg;

    io_arg = server->accept_func(socket, addr, &len, &sock_out);

    if (io_arg == NULL)
    {
        soap->error = SOAP_ERR;
        return -1;
    }
    *n = len;

    rc = xacml_request_init(&request);
    if (rc < 0)
    {
        soap->error = SOAP_ERR;
        return -1;
    }
    request->connect_func = server->connect_func;
    request->send_func = server->send_func;
    request->recv_func = server->recv_func;
    request->close_func = server->close_func;
    request->io_arg = io_arg;
    request->server = server;
    server->request = request;
    soap->user = request;

    return sock_out;
}
/* xacml_i_accept() */

#endif /* DONT_DOCUMENT_INTERNAL */
