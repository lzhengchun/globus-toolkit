/*
 * Copyright 1999-2008 University of Chicago
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

#define EXT_SIZE 16
#include "gssapi.h"
#include "globus_gss_assist.h"
#include "gssapi_openssl.h"

int verify_cred(
    gss_cred_id_t                       credential);

void globus_print_error(
    globus_result_t                     error_result);

int main()
{
    OM_uint32                           init_maj_stat;
    OM_uint32                           accept_maj_stat;
    OM_uint32                           maj_stat;
    OM_uint32                           min_stat;
    OM_uint32                           ret_flags;
    OM_uint32                           time_rec;
    OM_uint32                           local_min_stat;
    gss_buffer_desc                     send_tok;
    gss_buffer_desc                     recv_tok;
    gss_buffer_desc *                   token_ptr;
    gss_OID                             mech_type;
    gss_name_t                          target_name;
    gss_ctx_id_t                        init_context;
    gss_ctx_id_t                        accept_context;
    gss_ctx_id_t                        del_init_context;
    gss_ctx_id_t                        del_accept_context;
    gss_cred_id_t                       delegated_cred;
    gss_cred_id_t                       imported_cred;
    gss_cred_id_t                       cred_handle;
    char *                              error_str;

    /* Initialize variables */
    
    token_ptr = GSS_C_NO_BUFFER;
    init_context = GSS_C_NO_CONTEXT;
    accept_context = GSS_C_NO_CONTEXT;
    del_init_context = GSS_C_NO_CONTEXT;
    del_accept_context = GSS_C_NO_CONTEXT;
    delegated_cred = GSS_C_NO_CREDENTIAL;
    accept_maj_stat = GSS_S_CONTINUE_NEEDED;
    ret_flags = 0;

    /* Activate Modules */
    globus_module_activate(GLOBUS_GSI_GSS_ASSIST_MODULE);
    globus_module_activate(GLOBUS_GSI_GSSAPI_MODULE);

    /* acquire the credential */
    maj_stat = gss_acquire_cred(&min_stat,
                                NULL,
                                GSS_C_INDEFINITE,
                                GSS_C_NO_OID_SET,
                                GSS_C_BOTH,
                                &cred_handle,
                                NULL,
                                NULL);

    if(maj_stat != GSS_S_COMPLETE)
    {
        globus_gss_assist_display_status_str(&error_str,
                                             NULL,
                                             maj_stat,
                                             min_stat,
                                             0);
        printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
        globus_print_error((globus_result_t) min_stat);
        exit(1);
    }

    
    /* get the subject name */
    
    maj_stat = gss_inquire_cred(&min_stat, 
                                cred_handle,
                                &target_name,
                                NULL,
                                NULL,
                                NULL);

    if(maj_stat != GSS_S_COMPLETE)
    {
        globus_gss_assist_display_status_str(&error_str,
                                             NULL,
                                             maj_stat,
                                             min_stat,
                                             0);
        printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
        globus_print_error((globus_result_t) min_stat);
        exit(1);
    }


    /* set up the first security context */
    
    init_maj_stat = gss_init_sec_context(&min_stat,
                                         cred_handle,
                                         &init_context,
                                         target_name,
                                         GSS_C_NULL_OID,
                                         0,
                                         0,
                                         GSS_C_NO_CHANNEL_BINDINGS,
                                         token_ptr,
                                         NULL,
                                         &send_tok,
                                         NULL,
                                         NULL);


    if(init_maj_stat != GSS_S_CONTINUE_NEEDED)
    {
        globus_gss_assist_display_status_str(&error_str,
                                             NULL,
                                             init_maj_stat,
                                             min_stat,
                                             0);
        printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
        globus_print_error((globus_result_t) min_stat);
        exit(1);
    }

    while(1)
    {
        
        accept_maj_stat=gss_accept_sec_context(&min_stat,
                                               &accept_context,
                                               GSS_C_NO_CREDENTIAL,
                                               &send_tok, 
                                               GSS_C_NO_CHANNEL_BINDINGS,
                                               NULL,
                                               &mech_type,
                                               &recv_tok,
                                               &ret_flags,
                                               /* ignore time_rec */
                                               NULL, 
                                               NULL);

        gss_release_buffer(&local_min_stat, &send_tok);

        if(accept_maj_stat != GSS_S_COMPLETE &&
           accept_maj_stat != GSS_S_CONTINUE_NEEDED)
        {
            globus_gss_assist_display_status_str(&error_str,
                                                 NULL,
                                                 init_maj_stat,
                                                 min_stat,
                                                 0);
            printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
            globus_print_error((globus_result_t) min_stat);
            exit(1);
        }
        else if(accept_maj_stat == GSS_S_COMPLETE)
        {
            break;
        }

        init_maj_stat = gss_init_sec_context(&min_stat,
                                             GSS_C_NO_CREDENTIAL,
                                             &init_context,
                                             target_name,
                                             GSS_C_NULL_OID,
                                             0,
                                             0,
                                             GSS_C_NO_CHANNEL_BINDINGS,
                                             &recv_tok,
                                             NULL,
                                             &send_tok,
                                             NULL,
                                             NULL);
        
        
        gss_release_buffer(&local_min_stat, &recv_tok);
        if(init_maj_stat != GSS_S_COMPLETE &&
           init_maj_stat != GSS_S_CONTINUE_NEEDED)
        {
            globus_gss_assist_display_status_str(&error_str,
                                                 NULL,
                                                 init_maj_stat,
                                                 min_stat,
                                                 0);
            printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
            globus_print_error((globus_result_t) min_stat);
            exit(1);
        }
    }

    printf("%s:%d: Successfully established initial security context\n",
           __FILE__,
           __LINE__);


    /* delegate our credential over the initial security context and
     * insert a restriction extension into the delegated credential.
     * This is a post GT 2.0 feature.
     */
    init_maj_stat = gss_init_delegation(&min_stat,
                                        init_context,
                                        cred_handle,
                                        GSS_C_NO_OID,
                                        GSS_C_NO_OID_SET,
                                        GSS_C_NO_BUFFER_SET,
                                        token_ptr,
                                        0,
                                        0,
                                        &send_tok);
    

    if(init_maj_stat != GSS_S_COMPLETE &&
       init_maj_stat != GSS_S_CONTINUE_NEEDED)
    {
        globus_gss_assist_display_status_str(&error_str,
                                             NULL,
                                             init_maj_stat,
                                             min_stat,
                                             0);
        printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
        globus_print_error((globus_result_t) min_stat);
        exit(1);
    }

    while(1)
    {
        accept_maj_stat=gss_accept_delegation(&min_stat,
                                              accept_context,
                                              GSS_C_NO_OID_SET,
                                              GSS_C_NO_BUFFER_SET,
                                              &send_tok,
                                              0,
                                              0,
                                              &time_rec,
                                              &delegated_cred,
                                              &mech_type,
                                              &recv_tok);
        
        if(accept_maj_stat != GSS_S_COMPLETE &&
           accept_maj_stat != GSS_S_CONTINUE_NEEDED)
        {
            globus_gss_assist_display_status_str(&error_str,
                                                 NULL,
                                                 init_maj_stat,
                                                 min_stat,
                                                 0);
            printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
            globus_print_error((globus_result_t) min_stat);
            exit(1);
        }
        else if(accept_maj_stat == GSS_S_COMPLETE)
        {
            break;
        }

        init_maj_stat = gss_init_delegation(&min_stat,
                                            init_context,
                                            cred_handle,
                                            GSS_C_NO_OID,
                                            GSS_C_NO_OID_SET,
                                            GSS_C_NO_BUFFER_SET,
                                            &recv_tok,
                                            0,
                                            0,
                                            &send_tok);


        if(init_maj_stat != GSS_S_COMPLETE &&
           init_maj_stat != GSS_S_CONTINUE_NEEDED)
        {
            globus_gss_assist_display_status_str(&error_str,
                                                 NULL,
                                                 init_maj_stat,
                                                 min_stat,
                                                 0);
            printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
            globus_print_error((globus_result_t) min_stat);
            exit(1);
        }
    }
    
    printf("%s:%d: Successfully delegated credential\n",
           __FILE__,
           __LINE__);

    /* export and import the delegated credential */
    /* this can be done both to a buffer and to a file */
    /* New in GT 2.0 */

    maj_stat = gss_export_cred(&min_stat,
                               delegated_cred,
                               GSS_C_NO_OID,
                               0,
                               &send_tok);

    if(maj_stat != GSS_S_COMPLETE)
    {
        globus_gss_assist_display_status_str(&error_str,
                                             NULL,
                                             init_maj_stat,
                                             min_stat,
                                             0);
        printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
        globus_print_error((globus_result_t) min_stat);
        exit(1);
    }

    /* Check delegated proxy type--should be RFC-compliant */
    {
        globus_result_t                     result;
        globus_gsi_cred_handle_t            cred_handle;
        BIO                                *b;
        globus_gsi_cert_utils_cert_type_t   type;


        result = globus_gsi_cred_handle_init(&cred_handle, NULL);

        if (result != GLOBUS_SUCCESS)
        {
            printf("\nLINE %d ERROR: \%s\n\n",
                   __LINE__,
                   globus_error_print_friendly(globus_error_peek(result)));
            exit(1);
        }

        b = BIO_new(BIO_s_mem());

        BIO_write(b, send_tok.value, send_tok.length);

        globus_gsi_cred_read_proxy_bio(cred_handle, b);

        BIO_free(b);

        result = globus_gsi_cred_get_cert_type(cred_handle, &type);
        if (result != GLOBUS_SUCCESS)
        {
            printf("\nLINE %d ERROR: \%s\n\n",
                   __LINE__,
                   globus_error_print_friendly(globus_error_peek(result)));
            exit(1);
        }
        if (type != GLOBUS_GSI_CERT_UTILS_TYPE_RFC_IMPERSONATION_PROXY)
        {
            printf("\nLINE %d ERROR: Expected RFC Impersonation proxy, got %d\n",
                    __LINE__,
                    (int) type);
            exit(1);
        }

        printf("%s:%d: Successfully got expected RFC proxy type\n",
               __FILE__,
               __LINE__);
        globus_gsi_cred_handle_destroy(cred_handle);
    }

    
    maj_stat = gss_import_cred(&min_stat,
                               &imported_cred,
                               GSS_C_NO_OID,
                               0,
                               &send_tok,
                               0,
                               &time_rec);


    if(maj_stat != GSS_S_COMPLETE)
    {
        globus_gss_assist_display_status_str(&error_str,
                                             NULL,
                                             init_maj_stat,
                                             min_stat,
                                             0);
        printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
        globus_print_error((globus_result_t) min_stat);
        exit(1);
    }

    maj_stat = gss_release_buffer(&min_stat, &send_tok);
    if(maj_stat != GSS_S_COMPLETE)
    {
        globus_gss_assist_display_status_str(&error_str,
                                             NULL,
                                             init_maj_stat,
                                             min_stat,
                                             0);
        printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
        globus_print_error((globus_result_t) min_stat);
        exit(1);
    }

    printf("%s:%d: Successfully exported/imported the delegated credential\n",
           __FILE__,
           __LINE__);

    /* set up another security context using the delegated credential */
    init_maj_stat = gss_init_sec_context(&min_stat,
                                         imported_cred,
                                         &del_init_context,
                                         target_name,
                                         GSS_C_NULL_OID,
                                         0,
                                         0,
                                         GSS_C_NO_CHANNEL_BINDINGS,
                                         token_ptr,
                                         NULL,
                                         &send_tok,
                                         NULL,
                                         NULL);


    if(init_maj_stat != GSS_S_COMPLETE &&
       init_maj_stat != GSS_S_CONTINUE_NEEDED)
    {
        globus_gss_assist_display_status_str(&error_str,
                                             NULL,
                                             init_maj_stat,
                                             min_stat,
                                             0);
        printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
        globus_print_error((globus_result_t) min_stat);
        exit(1);
    }


    
    while(1)
    {
        accept_maj_stat=gss_accept_sec_context(&min_stat,
                                               &del_accept_context,
                                               imported_cred,
                                               &send_tok, 
                                               GSS_C_NO_CHANNEL_BINDINGS,
                                               &target_name,
                                               &mech_type,
                                               &recv_tok,
                                               &ret_flags,
                                               /* ignore time_rec */
                                               NULL, 
                                               NULL);

        if(accept_maj_stat != GSS_S_COMPLETE &&
           accept_maj_stat != GSS_S_CONTINUE_NEEDED)
        {
            globus_gss_assist_display_status_str(&error_str,
                                                 NULL,
                                                 init_maj_stat,
                                                 min_stat,
                                                 0);
            printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
            globus_print_error((globus_result_t) min_stat);
            exit(1);
        }
        else if(accept_maj_stat == GSS_S_COMPLETE)
        {
            break;
        }
        
        init_maj_stat = gss_init_sec_context(&min_stat,
                                             imported_cred,
                                             &del_init_context,
                                             target_name,
                                             GSS_C_NULL_OID,
                                             0,
                                             0,
                                             GSS_C_NO_CHANNEL_BINDINGS,
                                             &recv_tok,
                                             NULL,
                                             &send_tok,
                                             NULL,
                                             NULL);
        
        
        if(init_maj_stat != GSS_S_COMPLETE &&
           init_maj_stat != GSS_S_CONTINUE_NEEDED)
        {
            globus_gss_assist_display_status_str(&error_str,
                                                 NULL,
                                                 init_maj_stat,
                                                 min_stat,
                                                 0);
            printf("\nLINE %d ERROR: %s\n\n", __LINE__, error_str);
            globus_print_error((globus_result_t) min_stat);
            exit(1);
        }
    }

    /* got sec context based on delegated cred now */
    maj_stat = gss_release_name(&min_stat, &target_name);
    maj_stat = gss_release_cred(&min_stat, &cred_handle);

    printf("%s:%d: Successfully established security context with delegated credential\n",
           __FILE__,
           __LINE__);

    globus_module_deactivate_all();

    exit(0);
}



int verify_cred(
    gss_cred_id_t                       credential)
{
    gss_cred_id_desc *                  cred_handle;
    X509 *                              cert;
    X509 *                              previous_cert;
    STACK_OF(X509) *                    cert_chain;
    int                                 cert_count;
    char *                              error_str;
    globus_object_t *                   error_obj;
    globus_result_t                     result;

    cert_count = 1;
    cred_handle = (gss_cred_id_desc *) credential;

    result = globus_gsi_cred_get_cert_chain(
        cred_handle->cred_handle,
        &cert_chain);
    if(result != GLOBUS_SUCCESS)
    {
        error_obj = globus_error_get(result);
        error_str = globus_error_print_chain(error_obj);
        fprintf(stderr, error_str);
        globus_libc_free(error_str);
        globus_object_free(error_obj);
        exit(1);
    }        
    
    if(cert_chain)
    {
        cert_count += sk_X509_num(cert_chain);
    }

    result = globus_gsi_cred_get_cert(cred_handle->cred_handle,
                                      &cert);
    if(result != GLOBUS_SUCCESS)
    {
        error_obj = globus_error_get(result);
        error_str = globus_error_print_chain(error_obj);
        fprintf(stderr, error_str);
        globus_libc_free(error_str);
        globus_object_free(error_obj);
        exit(1);
    }

    previous_cert=NULL;
    cert_count--;

    do
    {
        if(previous_cert != NULL)
        {
            if(!X509_verify(previous_cert, X509_get_pubkey(cert)))
            {
                return 0;
            }
        }

        previous_cert = cert;

    } while(cert_count-- &&
            (cert = sk_X509_value(cert_chain, cert_count)));

    return 1;
}


void globus_print_error(
    globus_result_t                     error_result)
{
    globus_object_t *                   error_obj = NULL;
    char *                              error_string = NULL;
    
    error_obj = globus_error_get(error_result);
    error_string = globus_error_print_chain(error_obj);
    globus_libc_fprintf(stderr, "%s\n", error_string);
    globus_libc_free(error_string);
    globus_object_free(error_obj);
}
