
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>


static ngx_int_t ngx_http_send_error_page(ngx_http_request_t *r,
    ngx_http_err_page_t *err_page);
static ngx_int_t ngx_http_send_special_response(ngx_http_request_t *r,
    ngx_http_core_loc_conf_t *clcf, ngx_uint_t err);
static ngx_int_t ngx_http_send_refresh(ngx_http_request_t *r);

static u_char ngx_http_error_tail[] = " ";


static u_char ngx_http_msie_padding[] = CRLF;


static u_char ngx_http_msie_refresh_head[] =
"<html><head><meta http-equiv=\"Refresh\" content=\"0; URL=";


static u_char ngx_http_msie_refresh_tail[] =
"\"></head><body></body></html>" CRLF;


static char ngx_http_error_301_page[] =
"<html>" CRLF
"<head><title>301 Moved Permanently</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>301 Moved Permanently</h1></center>" CRLF
;


static char ngx_http_error_302_page[] =
"<html>" CRLF
"<head><title>302 Found</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>302 Found</h1></center>" CRLF
;


static char ngx_http_error_303_page[] =
"<html>" CRLF
"<head><title>303 See Other</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>303 See Other</h1></center>" CRLF
;


static char ngx_http_error_307_page[] =
"<html>" CRLF
"<head><title>307 Temporary Redirect</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>307 Temporary Redirect</h1></center>" CRLF
;


static char ngx_http_error_400_page[] =
"<html>" CRLF
"<head><title>400 Bad Request</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>400 Bad Request</h1></center>" CRLF
;


static char ngx_http_error_401_page[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" CRLF
"<html xmlns=\"http://www.w3.org/1999/xhtml\">" CRLF
"<head>" CRLF
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"/>" CRLF
"<title>401 - Unauthorized: Access is denied due to invalid credentials.</title>" CRLF
"<style type=\"text/css\">" CRLF
"<!--" CRLF
"body{margin:0;font-size:.7em;font-family:Verdana, Arial, Helvetica, sans-serif;background:#EEEEEE;}" CRLF
"fieldset{padding:0 15px 10px 15px;} " CRLF
"h1{font-size:2.4em;margin:0;color:#FFF;}" CRLF
"h2{font-size:1.7em;margin:0;color:#CC0000;} " CRLF
"h3{font-size:1.2em;margin:10px 0 0 0;color:#000000;} " CRLF
"#header{width:96%;margin:0 0 0 0;padding:6px 2% 6px 2%;font-family:\"trebuchet MS\", Verdana, sans-serif;color:#FFF;" CRLF
"background-color:#555555;}" CRLF
"#content{margin:0 0 0 2%;position:relative;}" CRLF
".content-container{background:#FFF;width:96%;margin-top:8px;padding:10px;position:relative;}" CRLF
"-->" CRLF
"</style>" CRLF
"</head>" CRLF
"<body>" CRLF
"<div id=\"header\"><h1>Server Error</h1></div>" CRLF
"<div id=\"content\">" CRLF
" <div class=\"content-container\"><fieldset>" CRLF
"  <h2>401 - Unauthorized: Access is denied due to invalid credentials.</h2>" CRLF
"  <h3>You do not have permission to view this directory or page using the credentials that you supplied.</h3>" CRLF
" </fieldset></div>" CRLF
"</div>" CRLF
"</body>" CRLF
"</html>"
;


static char ngx_http_error_402_page[] =
"<html>" CRLF
"<head><title>402 Payment Required</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>402 Payment Required</h1></center>" CRLF
;


static char ngx_http_error_403_page[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" CRLF
"<html xmlns=\"http://www.w3.org/1999/xhtml\">" CRLF
"<head>" CRLF
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"/>" CRLF
"<title>403 - Forbidden: Access is denied.</title>" CRLF
"<style type=\"text/css\">" CRLF
"<!--" CRLF
"body{margin:0;font-size:.7em;font-family:Verdana, Arial, Helvetica, sans-serif;background:#EEEEEE;}" CRLF
"fieldset{padding:0 15px 10px 15px;} " CRLF
"h1{font-size:2.4em;margin:0;color:#FFF;}" CRLF
"h2{font-size:1.7em;margin:0;color:#CC0000;} " CRLF
"h3{font-size:1.2em;margin:10px 0 0 0;color:#000000;} " CRLF
"#header{width:96%;margin:0 0 0 0;padding:6px 2% 6px 2%;font-family:\"trebuchet MS\", Verdana, sans-serif;color:#FFF;" CRLF
"background-color:#555555;}" CRLF
"#content{margin:0 0 0 2%;position:relative;}" CRLF
".content-container{background:#FFF;width:96%;margin-top:8px;padding:10px;position:relative;}" CRLF
"-->" CRLF
"</style>" CRLF
"</head>" CRLF
"<body>" CRLF
"<div id=\"header\"><h1>Server Error</h1></div>" CRLF
"<div id=\"content\">" CRLF
" <div class=\"content-container\"><fieldset>" CRLF
"  <h2>403 - Forbidden: Access is denied.</h2>" CRLF
"  <h3>You do not have permission to view this directory or page using the credentials that you supplied.</h3>" CRLF
" </fieldset></div>" CRLF
"</div>" CRLF
"</body>" CRLF
"</html>"
;


static char ngx_http_error_404_page[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" CRLF
"<html xmlns=\"http://www.w3.org/1999/xhtml\">" CRLF
"<head>" CRLF
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"/>" CRLF
"<title>404 - File or directory not found.</title>" CRLF
"<style type=\"text/css\">" CRLF
"<!--" CRLF
"body{margin:0;font-size:.7em;font-family:Verdana, Arial, Helvetica, sans-serif;background:#EEEEEE;}" CRLF
"fieldset{padding:0 15px 10px 15px;} " CRLF
"h1{font-size:2.4em;margin:0;color:#FFF;}" CRLF
"h2{font-size:1.7em;margin:0;color:#CC0000;} " CRLF
"h3{font-size:1.2em;margin:10px 0 0 0;color:#000000;} " CRLF
"#header{width:96%;margin:0 0 0 0;padding:6px 2% 6px 2%;font-family:\"trebuchet MS\", Verdana, sans-serif;color:#FFF;" CRLF
"background-color:#555555;}" CRLF
"#content{margin:0 0 0 2%;position:relative;}" CRLF
".content-container{background:#FFF;width:96%;margin-top:8px;padding:10px;position:relative;}" CRLF
"-->" CRLF
"</style>" CRLF
"</head>" CRLF
"<body>" CRLF
"<div id=\"header\"><h1>Server Error</h1></div>" CRLF
"<div id=\"content\">" CRLF
" <div class=\"content-container\"><fieldset>" CRLF
"  <h2>404 - File or directory not found.</h2>" CRLF
"  <h3>The resource you are looking for might have been removed, had its name changed, or is temporarily unavailable.</h3>" CRLF
" </fieldset></div>" CRLF
"</div>" CRLF
"</body>" CRLF
"</html>"
;


static char ngx_http_error_405_page[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" CRLF
"<html xmlns=\"http://www.w3.org/1999/xhtml\">" CRLF
"<head>" CRLF
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"/>" CRLF
"<title>405 - HTTP verb used to access this page is not allowed.</title>" CRLF
"<style type=\"text/css\">" CRLF
"<!--" CRLF
"body{margin:0;font-size:.7em;font-family:Verdana, Arial, Helvetica, sans-serif;background:#EEEEEE;}" CRLF
"fieldset{padding:0 15px 10px 15px;} " CRLF
"h1{font-size:2.4em;margin:0;color:#FFF;}" CRLF
"h2{font-size:1.7em;margin:0;color:#CC0000;} " CRLF
"h3{font-size:1.2em;margin:10px 0 0 0;color:#000000;} " CRLF
"#header{width:96%;margin:0 0 0 0;padding:6px 2% 6px 2%;font-family:\"trebuchet MS\", Verdana, sans-serif;color:#FFF;" CRLF
"background-color:#555555;}" CRLF
"#content{margin:0 0 0 2%;position:relative;}" CRLF
".content-container{background:#FFF;width:96%;margin-top:8px;padding:10px;position:relative;}" CRLF
"-->" CRLF
"</style>" CRLF
"</head>" CRLF
"<body>" CRLF
"<div id=\"header\"><h1>Server Error</h1></div>" CRLF
"<div id=\"content\">" CRLF
" <div class=\"content-container\"><fieldset>" CRLF
"  <h2>405 - HTTP verb used to access this page is not allowed.</h2>" CRLF
"  <h3>The page you are looking for cannot be displayed because an invalid method (HTTP verb) was used to attempt access.</h3>" CRLF
" </fieldset></div>" CRLF
"</div>" CRLF
"</body>" CRLF
"</html>"
;


static char ngx_http_error_406_page[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" CRLF
"<html xmlns=\"http://www.w3.org/1999/xhtml\">" CRLF
"<head>" CRLF
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"/>" CRLF
"<title>406 - Client browser does not accept the MIME type of the requested page.</title>" CRLF
"<style type=\"text/css\">" CRLF
"<!--" CRLF
"body{margin:0;font-size:.7em;font-family:Verdana, Arial, Helvetica, sans-serif;background:#EEEEEE;}" CRLF
"fieldset{padding:0 15px 10px 15px;} " CRLF
"h1{font-size:2.4em;margin:0;color:#FFF;}" CRLF
"h2{font-size:1.7em;margin:0;color:#CC0000;} " CRLF
"h3{font-size:1.2em;margin:10px 0 0 0;color:#000000;} " CRLF
"#header{width:96%;margin:0 0 0 0;padding:6px 2% 6px 2%;font-family:\"trebuchet MS\", Verdana, sans-serif;color:#FFF;" CRLF
"background-color:#555555;}" CRLF
"#content{margin:0 0 0 2%;position:relative;}" CRLF
".content-container{background:#FFF;width:96%;margin-top:8px;padding:10px;position:relative;}" CRLF
"-->" CRLF
"</style>" CRLF
"</head>" CRLF
"<body>" CRLF
"<div id=\"header\"><h1>Server Error</h1></div>" CRLF
"<div id=\"content\">" CRLF
" <div class=\"content-container\"><fieldset>" CRLF
"  <h2>406 - Client browser does not accept the MIME type of the requested page.</h2>" CRLF
"  <h3>The page you are looking for cannot be opened by your browser because it has a file name extension that your browser does not accept.</h3>" CRLF
" </fieldset></div>" CRLF
"</div>" CRLF
"</body>" CRLF
"</html>"
;


static char ngx_http_error_408_page[] =
"<html>" CRLF
"<head><title>408 Request Time-out</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>408 Request Time-out</h1></center>" CRLF
;


static char ngx_http_error_409_page[] =
"<html>" CRLF
"<head><title>409 Conflict</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>409 Conflict</h1></center>" CRLF
;


static char ngx_http_error_410_page[] =
"<html>" CRLF
"<head><title>410 Gone</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>410 Gone</h1></center>" CRLF
;


static char ngx_http_error_411_page[] =
"<html>" CRLF
"<head><title>411 Length Required</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>411 Length Required</h1></center>" CRLF
;


static char ngx_http_error_412_page[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" CRLF
"<html xmlns=\"http://www.w3.org/1999/xhtml\">" CRLF
"<head>" CRLF
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"/>" CRLF
"<title>412 - Precondition set by the client failed when evaluated on the Web server.</title>" CRLF
"<style type=\"text/css\">" CRLF
"<!--" CRLF
"body{margin:0;font-size:.7em;font-family:Verdana, Arial, Helvetica, sans-serif;background:#EEEEEE;}" CRLF
"fieldset{padding:0 15px 10px 15px;} " CRLF
"h1{font-size:2.4em;margin:0;color:#FFF;}" CRLF
"h2{font-size:1.7em;margin:0;color:#CC0000;} " CRLF
"h3{font-size:1.2em;margin:10px 0 0 0;color:#000000;} " CRLF
"#header{width:96%;margin:0 0 0 0;padding:6px 2% 6px 2%;font-family:\"trebuchet MS\", Verdana, sans-serif;color:#FFF;" CRLF
"background-color:#555555;}" CRLF
"#content{margin:0 0 0 2%;position:relative;}" CRLF
".content-container{background:#FFF;width:96%;margin-top:8px;padding:10px;position:relative;}" CRLF
"-->" CRLF
"</style>" CRLF
"</head>" CRLF
"<body>" CRLF
"<div id=\"header\"><h1>Server Error</h1></div>" CRLF
"<div id=\"content\">" CRLF
" <div class=\"content-container\"><fieldset>" CRLF
"  <h2>412 - Precondition set by the client failed when evaluated on the Web server.</h2>" CRLF
"  <h3>The request was not completed due to preconditions that are set in the request header. Preconditions prevent the requested method from being applied to a resource other than the one intended. An example of a precondition is testing for expired content in the page cache of the client.</h3>" CRLF
" </fieldset></div>" CRLF
"</div>" CRLF
"</body>" CRLF
"</html>"
;


static char ngx_http_error_413_page[] =
"<html>" CRLF
"<head><title>413 Request Entity Too Large</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>413 Request Entity Too Large</h1></center>" CRLF
;


static char ngx_http_error_414_page[] =
"<html>" CRLF
"<head><title>414 Request-URI Too Large</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>414 Request-URI Too Large</h1></center>" CRLF
;


static char ngx_http_error_415_page[] =
"<html>" CRLF
"<head><title>415 Unsupported Media Type</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>415 Unsupported Media Type</h1></center>" CRLF
;


static char ngx_http_error_416_page[] =
"<html>" CRLF
"<head><title>416 Requested Range Not Satisfiable</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>416 Requested Range Not Satisfiable</h1></center>" CRLF
;


static char ngx_http_error_421_page[] =
"<html>" CRLF
"<head><title>421 Misdirected Request</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>421 Misdirected Request</h1></center>" CRLF
;


static char ngx_http_error_494_page[] =
"<html>" CRLF
"<head><title>400 Request Header Or Cookie Too Large</title></head>"
CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>400 Bad Request</h1></center>" CRLF
"<center>Request Header Or Cookie Too Large</center>" CRLF
;


static char ngx_http_error_495_page[] =
"<html>" CRLF
"<head><title>400 The SSL certificate error</title></head>"
CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>400 Bad Request</h1></center>" CRLF
"<center>The SSL certificate error</center>" CRLF
;


static char ngx_http_error_496_page[] =
"<html>" CRLF
"<head><title>400 No required SSL certificate was sent</title></head>"
CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>400 Bad Request</h1></center>" CRLF
"<center>No required SSL certificate was sent</center>" CRLF
;


static char ngx_http_error_497_page[] =
"<html>" CRLF
"<head><title>400 The plain HTTP request was sent to HTTPS port</title></head>"
CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>400 Bad Request</h1></center>" CRLF
"<center>The plain HTTP request was sent to HTTPS port</center>" CRLF
;


static char ngx_http_error_500_page[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" CRLF
"<html xmlns=\"http://www.w3.org/1999/xhtml\">" CRLF
"<head>" CRLF
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"/>" CRLF
"<title>500 - Internal server error.</title>" CRLF
"<style type=\"text/css\">" CRLF
"<!--" CRLF
"body{margin:0;font-size:.7em;font-family:Verdana, Arial, Helvetica, sans-serif;background:#EEEEEE;}" CRLF
"fieldset{padding:0 15px 10px 15px;} " CRLF
"h1{font-size:2.4em;margin:0;color:#FFF;}" CRLF
"h2{font-size:1.7em;margin:0;color:#CC0000;} " CRLF
"h3{font-size:1.2em;margin:10px 0 0 0;color:#000000;} " CRLF
"#header{width:96%;margin:0 0 0 0;padding:6px 2% 6px 2%;font-family:\"trebuchet MS\", Verdana, sans-serif;color:#FFF;" CRLF
"background-color:#555555;}" CRLF
"#content{margin:0 0 0 2%;position:relative;}" CRLF
".content-container{background:#FFF;width:96%;margin-top:8px;padding:10px;position:relative;}" CRLF
"-->" CRLF
"</style>" CRLF
"</head>" CRLF
"<body>" CRLF
"<div id=\"header\"><h1>Server Error</h1></div>" CRLF
"<div id=\"content\">" CRLF
" <div class=\"content-container\"><fieldset>" CRLF
"  <h2>500 - Internal server error.</h2>" CRLF
"  <h3>There is a problem with the resource you are looking for, and it cannot be displayed.</h3>" CRLF
" </fieldset></div>" CRLF
"</div>" CRLF
"</body>" CRLF
"</html>"
;


static char ngx_http_error_501_page[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" CRLF
"<html xmlns=\"http://www.w3.org/1999/xhtml\">" CRLF
"<head>" CRLF
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"/>" CRLF
"<title>501 - Header values specify a method that is not implemented.</title>" CRLF
"<style type=\"text/css\">" CRLF
"<!--" CRLF
"body{margin:0;font-size:.7em;font-family:Verdana, Arial, Helvetica, sans-serif;background:#EEEEEE;}" CRLF
"fieldset{padding:0 15px 10px 15px;} " CRLF
"h1{font-size:2.4em;margin:0;color:#FFF;}" CRLF
"h2{font-size:1.7em;margin:0;color:#CC0000;} " CRLF
"h3{font-size:1.2em;margin:10px 0 0 0;color:#000000;} " CRLF
"#header{width:96%;margin:0 0 0 0;padding:6px 2% 6px 2%;font-family:\"trebuchet MS\", Verdana, sans-serif;color:#FFF;" CRLF
"background-color:#555555;}" CRLF
"#content{margin:0 0 0 2%;position:relative;}" CRLF
".content-container{background:#FFF;width:96%;margin-top:8px;padding:10px;position:relative;}" CRLF
"-->" CRLF
"</style>" CRLF
"</head>" CRLF
"<body>" CRLF
"<div id=\"header\"><h1>Server Error</h1></div>" CRLF
"<div id=\"content\">" CRLF
" <div class=\"content-container\"><fieldset>" CRLF
"  <h2>501 - Header values specify a method that is not implemented.</h2>" CRLF
"  <h3>The page you are looking for cannot be displayed because a header value in the request does not match certain configuration settings on the Web server. For example, a request header might specify a POST to a static file that cannot be posted to, or specify a Transfer-Encoding value that cannot make use of compression.</h3>" CRLF
" </fieldset></div>" CRLF
"</div>" CRLF
"</body>" CRLF
"</html>"
;


static char ngx_http_error_502_page[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" CRLF
"<html xmlns=\"http://www.w3.org/1999/xhtml\">" CRLF
"<head>" CRLF
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"/>" CRLF
"<title>502 - Web server received an invalid response while acting as a gateway or proxy server.</title>" CRLF
"<style type=\"text/css\">" CRLF
"<!--" CRLF
"body{margin:0;font-size:.7em;font-family:Verdana, Arial, Helvetica, sans-serif;background:#EEEEEE;}" CRLF
"fieldset{padding:0 15px 10px 15px;} " CRLF
"h1{font-size:2.4em;margin:0;color:#FFF;}" CRLF
"h2{font-size:1.7em;margin:0;color:#CC0000;} " CRLF
"h3{font-size:1.2em;margin:10px 0 0 0;color:#000000;} " CRLF
"#header{width:96%;margin:0 0 0 0;padding:6px 2% 6px 2%;font-family:\"trebuchet MS\", Verdana, sans-serif;color:#FFF;" CRLF
"background-color:#555555;}" CRLF
"#content{margin:0 0 0 2%;position:relative;}" CRLF
".content-container{background:#FFF;width:96%;margin-top:8px;padding:10px;position:relative;}" CRLF
"-->" CRLF
"</style>" CRLF
"</head>" CRLF
"<body>" CRLF
"<div id=\"header\"><h1>Server Error</h1></div>" CRLF
"<div id=\"content\">" CRLF
" <div class=\"content-container\"><fieldset>" CRLF
"  <h2>502 - Web server received an invalid response while acting as a gateway or proxy server.</h2>" CRLF
"  <h3>There is a problem with the page you are looking for, and it cannot be displayed. When the Web server (while acting as a gateway or proxy) contacted the upstream content server, it received an invalid response from the content server.</h3>" CRLF
" </fieldset></div>" CRLF
"</div>" CRLF
"</body>" CRLF
"</html>"
;


static char ngx_http_error_503_page[] =
"<html>" CRLF
"<head><title>503 Service Temporarily Unavailable</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>503 Service Temporarily Unavailable</h1></center>" CRLF
;


static char ngx_http_error_504_page[] =
"<html>" CRLF
"<head><title>504 Gateway Time-out</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>504 Gateway Time-out</h1></center>" CRLF
;


static char ngx_http_error_507_page[] =
"<html>" CRLF
"<head><title>507 Insufficient Storage</title></head>" CRLF
"<body bgcolor=\"white\">" CRLF
"<center><h1>507 Insufficient Storage</h1></center>" CRLF
;


static ngx_str_t ngx_http_error_pages[] = {

    ngx_null_string,                     /* 201, 204 */

#define NGX_HTTP_LAST_2XX  202
#define NGX_HTTP_OFF_3XX   (NGX_HTTP_LAST_2XX - 201)

    /* ngx_null_string, */               /* 300 */
    ngx_string(ngx_http_error_301_page),
    ngx_string(ngx_http_error_302_page),
    ngx_string(ngx_http_error_303_page),
    ngx_null_string,                     /* 304 */
    ngx_null_string,                     /* 305 */
    ngx_null_string,                     /* 306 */
    ngx_string(ngx_http_error_307_page),

#define NGX_HTTP_LAST_3XX  308
#define NGX_HTTP_OFF_4XX   (NGX_HTTP_LAST_3XX - 301 + NGX_HTTP_OFF_3XX)

    ngx_string(ngx_http_error_400_page),
    ngx_string(ngx_http_error_401_page),
    ngx_string(ngx_http_error_402_page),
    ngx_string(ngx_http_error_403_page),
    ngx_string(ngx_http_error_404_page),
    ngx_string(ngx_http_error_405_page),
    ngx_string(ngx_http_error_406_page),
    ngx_null_string,                     /* 407 */
    ngx_string(ngx_http_error_408_page),
    ngx_string(ngx_http_error_409_page),
    ngx_string(ngx_http_error_410_page),
    ngx_string(ngx_http_error_411_page),
    ngx_string(ngx_http_error_412_page),
    ngx_string(ngx_http_error_413_page),
    ngx_string(ngx_http_error_414_page),
    ngx_string(ngx_http_error_415_page),
    ngx_string(ngx_http_error_416_page),
    ngx_null_string,                     /* 417 */
    ngx_null_string,                     /* 418 */
    ngx_null_string,                     /* 419 */
    ngx_null_string,                     /* 420 */
    ngx_string(ngx_http_error_421_page),

#define NGX_HTTP_LAST_4XX  422
#define NGX_HTTP_OFF_5XX   (NGX_HTTP_LAST_4XX - 400 + NGX_HTTP_OFF_4XX)

    ngx_string(ngx_http_error_494_page), /* 494, request header too large */
    ngx_string(ngx_http_error_495_page), /* 495, https certificate error */
    ngx_string(ngx_http_error_496_page), /* 496, https no certificate */
    ngx_string(ngx_http_error_497_page), /* 497, http to https */
    ngx_string(ngx_http_error_404_page), /* 498, canceled */
    ngx_null_string,                     /* 499, client has closed connection */

    ngx_string(ngx_http_error_500_page),
    ngx_string(ngx_http_error_501_page),
    ngx_string(ngx_http_error_502_page),
    ngx_string(ngx_http_error_503_page),
    ngx_string(ngx_http_error_504_page),
    ngx_null_string,                     /* 505 */
    ngx_null_string,                     /* 506 */
    ngx_string(ngx_http_error_507_page)

#define NGX_HTTP_LAST_5XX  508

};


ngx_int_t
ngx_http_special_response_handler(ngx_http_request_t *r, ngx_int_t error)
{
    ngx_uint_t                 i, err;
    ngx_http_err_page_t       *err_page;
    ngx_http_core_loc_conf_t  *clcf;

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http special response: %i, \"%V?%V\"",
                   error, &r->uri, &r->args);

    r->err_status = error;

    if (r->keepalive) {
        switch (error) {
            case NGX_HTTP_BAD_REQUEST:
            case NGX_HTTP_REQUEST_ENTITY_TOO_LARGE:
            case NGX_HTTP_REQUEST_URI_TOO_LARGE:
            case NGX_HTTP_TO_HTTPS:
            case NGX_HTTPS_CERT_ERROR:
            case NGX_HTTPS_NO_CERT:
            case NGX_HTTP_INTERNAL_SERVER_ERROR:
            case NGX_HTTP_NOT_IMPLEMENTED:
                r->keepalive = 0;
        }
    }

    if (r->lingering_close) {
        switch (error) {
            case NGX_HTTP_BAD_REQUEST:
            case NGX_HTTP_TO_HTTPS:
            case NGX_HTTPS_CERT_ERROR:
            case NGX_HTTPS_NO_CERT:
                r->lingering_close = 0;
        }
    }

    r->headers_out.content_type.len = 0;

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (!r->error_page && clcf->error_pages && r->uri_changes != 0) {

        if (clcf->recursive_error_pages == 0) {
            r->error_page = 1;
        }

        err_page = clcf->error_pages->elts;

        for (i = 0; i < clcf->error_pages->nelts; i++) {
            if (err_page[i].status == error) {
                return ngx_http_send_error_page(r, &err_page[i]);
            }
        }
    }

    r->expect_tested = 1;

    if (ngx_http_discard_request_body(r) != NGX_OK) {
        r->keepalive = 0;
    }

    if (clcf->msie_refresh
        && r->headers_in.msie
        && (error == NGX_HTTP_MOVED_PERMANENTLY
            || error == NGX_HTTP_MOVED_TEMPORARILY))
    {
        return ngx_http_send_refresh(r);
    }

    if (error == NGX_HTTP_CREATED) {
        /* 201 */
        err = 0;

    } else if (error == NGX_HTTP_NO_CONTENT) {
        /* 204 */
        err = 0;

    } else if (error >= NGX_HTTP_MOVED_PERMANENTLY
               && error < NGX_HTTP_LAST_3XX)
    {
        /* 3XX */
        err = error - NGX_HTTP_MOVED_PERMANENTLY + NGX_HTTP_OFF_3XX;

    } else if (error >= NGX_HTTP_BAD_REQUEST
               && error < NGX_HTTP_LAST_4XX)
    {
        /* 4XX */
        err = error - NGX_HTTP_BAD_REQUEST + NGX_HTTP_OFF_4XX;

    } else if (error >= NGX_HTTP_NGINX_CODES
               && error < NGX_HTTP_LAST_5XX)
    {
        /* 49X, 5XX */
        err = error - NGX_HTTP_NGINX_CODES + NGX_HTTP_OFF_5XX;
        switch (error) {
            case NGX_HTTP_TO_HTTPS:
            case NGX_HTTPS_CERT_ERROR:
            case NGX_HTTPS_NO_CERT:
            case NGX_HTTP_REQUEST_HEADER_TOO_LARGE:
                r->err_status = NGX_HTTP_BAD_REQUEST;
        }

    } else {
        /* unknown code, zero body */
        err = 0;
    }

    return ngx_http_send_special_response(r, clcf, err);
}


ngx_int_t
ngx_http_filter_finalize_request(ngx_http_request_t *r, ngx_module_t *m,
    ngx_int_t error)
{
    void       *ctx;
    ngx_int_t   rc;

    ngx_http_clean_header(r);

    ctx = NULL;

    if (m) {
        ctx = r->ctx[m->ctx_index];
    }

    /* clear the modules contexts */
    ngx_memzero(r->ctx, sizeof(void *) * ngx_http_max_module);

    if (m) {
        r->ctx[m->ctx_index] = ctx;
    }

    r->filter_finalize = 1;

    rc = ngx_http_special_response_handler(r, error);

    /* NGX_ERROR resets any pending data */

    switch (rc) {

    case NGX_OK:
    case NGX_DONE:
        return NGX_ERROR;

    default:
        return rc;
    }
}


void
ngx_http_clean_header(ngx_http_request_t *r)
{
    ngx_memzero(&r->headers_out.status,
                sizeof(ngx_http_headers_out_t)
                    - offsetof(ngx_http_headers_out_t, status));

    r->headers_out.headers.part.nelts = 0;
    r->headers_out.headers.part.next = NULL;
    r->headers_out.headers.last = &r->headers_out.headers.part;

    r->headers_out.content_length_n = -1;
    r->headers_out.last_modified_time = -1;
}


static ngx_int_t
ngx_http_send_error_page(ngx_http_request_t *r, ngx_http_err_page_t *err_page)
{
    ngx_int_t                  overwrite;
    ngx_str_t                  uri, args;
    ngx_table_elt_t           *location;
    ngx_http_core_loc_conf_t  *clcf;

    overwrite = err_page->overwrite;

    if (overwrite && overwrite != NGX_HTTP_OK) {
        r->expect_tested = 1;
    }

    if (overwrite >= 0) {
        r->err_status = overwrite;
    }

    if (ngx_http_complex_value(r, &err_page->value, &uri) != NGX_OK) {
        return NGX_ERROR;
    }

    if (uri.len && uri.data[0] == '/') {

        if (err_page->value.lengths) {
            ngx_http_split_args(r, &uri, &args);

        } else {
            args = err_page->args;
        }

        if (r->method != NGX_HTTP_HEAD) {
            r->method = NGX_HTTP_GET;
            r->method_name = ngx_http_core_get_method;
        }

        return ngx_http_internal_redirect(r, &uri, &args);
    }

    if (uri.len && uri.data[0] == '@') {
        return ngx_http_named_location(r, &uri);
    }

    location = ngx_list_push(&r->headers_out.headers);

    if (location == NULL) {
        return NGX_ERROR;
    }

    if (overwrite != NGX_HTTP_MOVED_PERMANENTLY
        && overwrite != NGX_HTTP_MOVED_TEMPORARILY
        && overwrite != NGX_HTTP_SEE_OTHER
        && overwrite != NGX_HTTP_TEMPORARY_REDIRECT)
    {
        r->err_status = NGX_HTTP_MOVED_TEMPORARILY;
    }

    location->hash = 1;
    ngx_str_set(&location->key, "Location");
    location->value = uri;

    ngx_http_clear_location(r);

    r->headers_out.location = location;

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (clcf->msie_refresh && r->headers_in.msie) {
        return ngx_http_send_refresh(r);
    }

    return ngx_http_send_special_response(r, clcf, r->err_status
                                                   - NGX_HTTP_MOVED_PERMANENTLY
                                                   + NGX_HTTP_OFF_3XX);
}


static ngx_int_t
ngx_http_send_special_response(ngx_http_request_t *r,
    ngx_http_core_loc_conf_t *clcf, ngx_uint_t err)
{
    u_char       *tail;
    size_t        len;
    ngx_int_t     rc;
    ngx_buf_t    *b;
    ngx_uint_t    msie_padding;
    ngx_chain_t   out[3];

    len = sizeof(ngx_http_error_tail) - 1;
    tail = ngx_http_error_tail;

    msie_padding = 0;

    if (ngx_http_error_pages[err].len) {
        r->headers_out.content_length_n = ngx_http_error_pages[err].len + len;
        if (clcf->msie_padding
            && (r->headers_in.msie || r->headers_in.chrome)
            && r->http_version >= NGX_HTTP_VERSION_10
            && err >= NGX_HTTP_OFF_4XX)
        {
            r->headers_out.content_length_n +=
                                         sizeof(ngx_http_msie_padding) - 1;
            msie_padding = 1;
        }

        r->headers_out.content_type_len = sizeof("text/html") - 1;
        ngx_str_set(&r->headers_out.content_type, "text/html");
        r->headers_out.content_type_lowcase = NULL;

    } else {
        r->headers_out.content_length_n = 0;
    }

    if (r->headers_out.content_length) {
        r->headers_out.content_length->hash = 0;
        r->headers_out.content_length = NULL;
    }

    ngx_http_clear_accept_ranges(r);
    ngx_http_clear_last_modified(r);
    ngx_http_clear_etag(r);

    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || r->header_only) {
        return rc;
    }

    if (ngx_http_error_pages[err].len == 0) {
        return ngx_http_send_special(r, NGX_HTTP_LAST);
    }

    b = ngx_calloc_buf(r->pool);
    if (b == NULL) {
        return NGX_ERROR;
    }

    b->memory = 1;
    b->pos = ngx_http_error_pages[err].data;
    b->last = ngx_http_error_pages[err].data + ngx_http_error_pages[err].len;

    out[0].buf = b;
    out[0].next = &out[1];

    b = ngx_calloc_buf(r->pool);
    if (b == NULL) {
        return NGX_ERROR;
    }

    b->memory = 1;

    b->pos = tail;
    b->last = tail + len;

    out[1].buf = b;
    out[1].next = NULL;

    if (msie_padding) {
        b = ngx_calloc_buf(r->pool);
        if (b == NULL) {
            return NGX_ERROR;
        }

        b->memory = 1;
        b->pos = ngx_http_msie_padding;
        b->last = ngx_http_msie_padding + sizeof(ngx_http_msie_padding) - 1;

        out[1].next = &out[2];
        out[2].buf = b;
        out[2].next = NULL;
    }

    if (r == r->main) {
        b->last_buf = 1;
    }

    b->last_in_chain = 1;

    return ngx_http_output_filter(r, &out[0]);
}


static ngx_int_t
ngx_http_send_refresh(ngx_http_request_t *r)
{
    u_char       *p, *location;
    size_t        len, size;
    uintptr_t     escape;
    ngx_int_t     rc;
    ngx_buf_t    *b;
    ngx_chain_t   out;

    len = r->headers_out.location->value.len;
    location = r->headers_out.location->value.data;

    escape = 2 * ngx_escape_uri(NULL, location, len, NGX_ESCAPE_REFRESH);

    size = sizeof(ngx_http_msie_refresh_head) - 1
           + escape + len
           + sizeof(ngx_http_msie_refresh_tail) - 1;

    r->err_status = NGX_HTTP_OK;

    r->headers_out.content_type_len = sizeof("text/html") - 1;
    ngx_str_set(&r->headers_out.content_type, "text/html");
    r->headers_out.content_type_lowcase = NULL;

    r->headers_out.location->hash = 0;
    r->headers_out.location = NULL;

    r->headers_out.content_length_n = size;

    if (r->headers_out.content_length) {
        r->headers_out.content_length->hash = 0;
        r->headers_out.content_length = NULL;
    }

    ngx_http_clear_accept_ranges(r);
    ngx_http_clear_last_modified(r);
    ngx_http_clear_etag(r);

    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || r->header_only) {
        return rc;
    }

    b = ngx_create_temp_buf(r->pool, size);
    if (b == NULL) {
        return NGX_ERROR;
    }

    p = ngx_cpymem(b->pos, ngx_http_msie_refresh_head,
                   sizeof(ngx_http_msie_refresh_head) - 1);

    if (escape == 0) {
        p = ngx_cpymem(p, location, len);

    } else {
        p = (u_char *) ngx_escape_uri(p, location, len, NGX_ESCAPE_REFRESH);
    }

    b->last = ngx_cpymem(p, ngx_http_msie_refresh_tail,
                         sizeof(ngx_http_msie_refresh_tail) - 1);

    b->last_buf = (r == r->main) ? 1 : 0;
    b->last_in_chain = 1;

    out.buf = b;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}
