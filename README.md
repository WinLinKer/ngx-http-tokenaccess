# ngxTokenAccess

## Introduction
A nginx module to control static resources's access through a valid token. The token, usually parsed from url argments, is the only key to access value (like static file name) stored in redis/memcached.

## example usage

	location ~ ^/static/\d+.daily/\d+_mail.htm$ {
	    root /home/mgsys/dev/;
	    tokenaccess on;
	    token_key token;
	    redis_url 192.168.1.23:6379;
	}
