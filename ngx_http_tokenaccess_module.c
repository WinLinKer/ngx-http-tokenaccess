#include <ngx_config.h>#include <ngx_core.h>#include <ngx_http.h>#include <time.h>typedef struct {	ngx_flag_t  enable;	ngx_str_t   token_key;	ngx_str_t   redis_pass;} ngx_http_tokenaccess_loc_conf_t;typedef struct {	ngx_str_t   response_value;	time_t		s_time;	time_t		e_time;	double 		diff_t;} ngx_http_tokenaccess_ctx_t;static ngx_int_t ngx_http_tokenaccess_subrequest_post_handler(ngx_http_request_t *r, void *data, ngx_int_t rc);static void ngx_http_tokenaccess_post_handler(ngx_http_request_t *r);static char *ngx_http_tokenaccess(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);static void *ngx_http_tokenaccess_create_loc_conf(ngx_conf_t *cf);static char *ngx_http_tokenaccess_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);static ngx_command_t ngx_http_tokenaccess_commands[] = {	{ ngx_string("tokenaccess"),	  NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,	  ngx_conf_set_flag_slot,	  NGX_HTTP_LOC_CONF_OFFSET,	  offsetof(ngx_http_tokenaccess_loc_conf_t, enable),	  NULL },	{ ngx_string("token_key"),	  NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,	  ngx_conf_set_str_slot,	  NGX_HTTP_LOC_CONF_OFFSET,	  offsetof(ngx_http_tokenaccess_loc_conf_t, token_key),	  NULL },	{ ngx_string("redis_pass"),	  NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,	 //ngx_conf_set_str_slot,	  ngx_http_tokenaccess,	  NGX_HTTP_LOC_CONF_OFFSET,	  offsetof(ngx_http_tokenaccess_loc_conf_t, redis_pass),	  NULL },	ngx_null_command};static ngx_http_module_t ngx_http_tokenaccess_module_ctx = {    NULL,                                /* preconfiguration */    NULL,                				 /* postconfiguration */    NULL,                                /* create main configuration */    NULL,                                /* init main configuration */    NULL,                                /* create server configuration */    NULL,                                /* merge server configuration */    ngx_http_tokenaccess_create_loc_conf,     /* create location configuration */    ngx_http_tokenaccess_merge_loc_conf       /* merge location configuration */};ngx_module_t ngx_http_tokenaccess_module = {    NGX_MODULE_V1,    &ngx_http_tokenaccess_module_ctx,         /* module context */    ngx_http_tokenaccess_commands,            /* module directives */    NGX_HTTP_MODULE,                     /* module type */    NULL,                                /* init master */    NULL,                                /* init module */    NULL,                                /* init process */    NULL,                                /* init thread */    NULL,                                /* exit thread */    NULL,                                /* exit process */    NULL,                                /* exit master */    NGX_MODULE_V1_PADDING};static ngx_int_t ngx_http_tokenaccess_subrequest_post_handler(ngx_http_request_t *r, void *data, ngx_int_t rc) {	ngx_http_request_t *pr = r->parent;	ngx_http_tokenaccess_ctx_t *ctx = ngx_http_get_module_ctx(pr, ngx_http_tokenaccess_module);	pr->headers_out.status = r->headers_out.status;	if (pr->headers_out.status == NGX_HTTP_OK) {		ngx_buf_t *pRecvBuf = &r->upstream->buffer;		ctx->response_value.len = pRecvBuf->last - pRecvBuf->pos;		ctx->response_value.data = pRecvBuf->pos;	}	pr->write_event_handler = ngx_http_tokenaccess_post_handler;	return NGX_OK;}static void ngx_http_tokenaccess_post_handler(ngx_http_request_t *r){	// a hack to make sure http request count larger than zero	// if it is zero, the next client request will not be able to sent	r->count = 1; 		if (r->headers_out.status != NGX_HTTP_OK) {		ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);		return;	}	ngx_http_tokenaccess_ctx_t *ctx = ngx_http_get_module_ctx(r, ngx_http_tokenaccess_module);	/* parse the url arguments: file name */	u_char response_value[1024] = {0};	u_char file_name[1024] = {0};	ngx_uint_t i=0, s=0, v=0;	for (i = 0; i <= r->uri.len; i++){		if ( s>0 && i == r->uri.len ) {			v = i;		} else if ( v==0 && r->uri.data[i] == '/' ) {			s = i+1;		} 	}	if (s == v) {		ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "file name is undefined!");		ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);        return;	}	ngx_memcpy(file_name, &r->uri.data[s], v-s);		ngx_memcpy(response_value, &ctx->response_value.data[0], ctx->response_value.len-1);	if ( ngx_strncmp(file_name, response_value, ngx_strlen(response_value)) != 0 ) {		ngx_http_finalize_request(r, NGX_HTTP_NOT_ALLOWED);		return;	}	sleep(2); // create a fake delay	time(&ctx->e_time);	ctx->diff_t = difftime(ctx->e_time, ctx->s_time)*1000;	ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "total time spent: %f ms\n", ctx->diff_t);	ngx_http_finalize_request(r, NGX_DECLINED);}static ngx_int_t ngx_http_tokenaccess_handler(ngx_http_request_t *r){	ngx_http_tokenaccess_loc_conf_t *my_cnf;	ngx_int_t rc;	ngx_str_t args = r->args;	ngx_log_t *log;	u_char token_key[1024] = {0};	if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))){		return NGX_HTTP_NOT_ALLOWED;	}	if (r->uri.data[r->uri.len - 1] == '/') {        return NGX_DECLINED;    }	my_cnf = ngx_http_get_module_loc_conf(r, ngx_http_tokenaccess_module);	if (!my_cnf->enable){		return NGX_DECLINED;	}	log = r->connection->log;	rc = ngx_http_discard_request_body(r);	if (rc != NGX_OK){		return rc;	}	ngx_str_t look = my_cnf->token_key;	if (look.len == 0){		ngx_log_error(NGX_LOG_EMERG, log, 0, "token key is undefined!");        return NGX_HTTP_INTERNAL_SERVER_ERROR;	}	/* parse the url arguments: token_key */	ngx_uint_t i, j=0, k=0, l=0;	for (i = 0; i <= args.len; i++) {		if ( (i == args.len) || (args.data[i] == '&') ) {			if (j > 1 ){				k = j;				l = i;			}			j = 0;		} else if ( (j == 0) &&  (i<args.len-look.len) ){			if ( (ngx_strncmp(args.data+i, look.data, look.len) == 0) 					&& (args.data[i+look.len] == '=')) {				j=i+look.len+1;				i=j-1;			}else j=1;		}	}	if (k == l){		ngx_log_error(NGX_LOG_EMERG, log, 0, "token value is undefined!");        return NGX_HTTP_BAD_REQUEST;	}	ngx_memcpy(token_key, &args.data[k], l-k);	/* start the sub-request part */	ngx_http_tokenaccess_ctx_t *ctx = ngx_http_get_module_ctx(r, ngx_http_tokenaccess_module);	if (ctx == NULL) {		ctx = ngx_palloc(r->pool, sizeof(ngx_http_tokenaccess_ctx_t));		if (ctx == NULL) {			return NGX_ERROR;		}		time(&ctx->s_time);		ngx_http_set_ctx(r, ctx, ngx_http_tokenaccess_module);	}	ngx_http_post_subrequest_t *psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));	if (psr == NULL) {		return NGX_HTTP_INTERNAL_SERVER_ERROR;	}	psr->handler = ngx_http_tokenaccess_subrequest_post_handler;	psr->data = ctx;	// make up a restful subrequest url	ngx_str_t sub_prefix = my_cnf->redis_pass;	ngx_str_t sub_location;	sub_location.len = sub_prefix.len + ngx_strlen(token_key) + 1;	sub_location.data = ngx_palloc(r->pool, sub_location.len);	ngx_snprintf(sub_location.data, sub_location.len, "%V/%s", &sub_prefix, token_key);		ngx_http_request_t *sr;	rc = ngx_http_subrequest(r, &sub_location, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);	if (rc != NGX_OK) {		return NGX_ERROR;	}	return NGX_DONE;}static void *ngx_http_tokenaccess_create_loc_conf(ngx_conf_t *cf){	ngx_http_tokenaccess_loc_conf_t *conf = NULL;	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tokenaccess_loc_conf_t));	if (conf == NULL) {		return NULL;	}    conf->enable = NGX_CONF_UNSET;    ngx_str_null(&conf->token_key);    ngx_str_null(&conf->redis_pass);    return conf;}static char *ngx_http_tokenaccess_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child){    ngx_http_tokenaccess_loc_conf_t  *prev = parent;     ngx_http_tokenaccess_loc_conf_t  *conf = child;		ngx_conf_merge_value(conf->enable, prev->enable, 0);	ngx_conf_merge_str_value(conf->token_key, prev->token_key, "token_key");	ngx_conf_merge_str_value(conf->redis_pass, prev->redis_pass, "redis_pass");	return NGX_CONF_OK;}static char *ngx_http_tokenaccess(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {    ngx_http_core_loc_conf_t *clcf;    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);    clcf->handler = ngx_http_tokenaccess_handler;    ngx_conf_set_str_slot(cf, cmd, conf);    return NGX_CONF_OK;}