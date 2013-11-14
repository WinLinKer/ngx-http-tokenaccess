# ngxTokenAccess

## Introduction
A nginx module to control static resources's access through a valid token. The token, usually parsed from url argments, is the only key to access value (like static file name) stored in redis/memcached.

## example usage
	server {
		listen 8080;

		location ~ ^/static/\d+.daily/\d+_mail.htm$ {
	    	root /home/mgsys/dev/;
		    tokenaccess on;
	        token_key token;
		    redis_pass /token_validation;
		}

		location /token_validation {
		    proxy_pass http://127.0.0.1:8000;
		    proxy_set_header Accept-Encoding "";
		}

	}

	server {
		listen 8000;

		location /token_validation {
		    default_type text/plain;
	        echo "a token value"; 
		}
		
	}

