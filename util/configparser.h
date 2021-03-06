/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_UTIL_CONFIGPARSER_H_INCLUDED
# define YY_YY_UTIL_CONFIGPARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    SPACE = 258,
    LETTER = 259,
    NEWLINE = 260,
    COMMENT = 261,
    COLON = 262,
    ANY = 263,
    ZONESTR = 264,
    STRING_ARG = 265,
    VAR_SERVER = 266,
    VAR_VERBOSITY = 267,
    VAR_NUM_THREADS = 268,
    VAR_PORT = 269,
    VAR_OUTGOING_RANGE = 270,
    VAR_INTERFACE = 271,
    VAR_DO_IP4 = 272,
    VAR_DO_IP6 = 273,
    VAR_DO_UDP = 274,
    VAR_DO_TCP = 275,
    VAR_TCP_MSS = 276,
    VAR_OUTGOING_TCP_MSS = 277,
    VAR_CHROOT = 278,
    VAR_USERNAME = 279,
    VAR_DIRECTORY = 280,
    VAR_LOGFILE = 281,
    VAR_PIDFILE = 282,
    VAR_MSG_CACHE_SIZE = 283,
    VAR_MSG_CACHE_SLABS = 284,
    VAR_NUM_QUERIES_PER_THREAD = 285,
    VAR_RRSET_CACHE_SIZE = 286,
    VAR_RRSET_CACHE_SLABS = 287,
    VAR_OUTGOING_NUM_TCP = 288,
    VAR_INFRA_HOST_TTL = 289,
    VAR_INFRA_LAME_TTL = 290,
    VAR_INFRA_CACHE_SLABS = 291,
    VAR_INFRA_CACHE_NUMHOSTS = 292,
    VAR_INFRA_CACHE_LAME_SIZE = 293,
    VAR_NAME = 294,
    VAR_STUB_ZONE = 295,
    VAR_STUB_HOST = 296,
    VAR_STUB_ADDR = 297,
    VAR_TARGET_FETCH_POLICY = 298,
    VAR_HARDEN_SHORT_BUFSIZE = 299,
    VAR_HARDEN_LARGE_QUERIES = 300,
    VAR_FORWARD_ZONE = 301,
    VAR_FORWARD_HOST = 302,
    VAR_FORWARD_ADDR = 303,
    VAR_DO_NOT_QUERY_ADDRESS = 304,
    VAR_HIDE_IDENTITY = 305,
    VAR_HIDE_VERSION = 306,
    VAR_IDENTITY = 307,
    VAR_VERSION = 308,
    VAR_HARDEN_GLUE = 309,
    VAR_MODULE_CONF = 310,
    VAR_TRUST_ANCHOR_FILE = 311,
    VAR_TRUST_ANCHOR = 312,
    VAR_VAL_OVERRIDE_DATE = 313,
    VAR_BOGUS_TTL = 314,
    VAR_VAL_CLEAN_ADDITIONAL = 315,
    VAR_VAL_PERMISSIVE_MODE = 316,
    VAR_INCOMING_NUM_TCP = 317,
    VAR_MSG_BUFFER_SIZE = 318,
    VAR_KEY_CACHE_SIZE = 319,
    VAR_KEY_CACHE_SLABS = 320,
    VAR_TRUSTED_KEYS_FILE = 321,
    VAR_VAL_NSEC3_KEYSIZE_ITERATIONS = 322,
    VAR_USE_SYSLOG = 323,
    VAR_OUTGOING_INTERFACE = 324,
    VAR_ROOT_HINTS = 325,
    VAR_DO_NOT_QUERY_LOCALHOST = 326,
    VAR_CACHE_MAX_TTL = 327,
    VAR_HARDEN_DNSSEC_STRIPPED = 328,
    VAR_ACCESS_CONTROL = 329,
    VAR_LOCAL_ZONE = 330,
    VAR_LOCAL_DATA = 331,
    VAR_INTERFACE_AUTOMATIC = 332,
    VAR_STATISTICS_INTERVAL = 333,
    VAR_DO_DAEMONIZE = 334,
    VAR_USE_CAPS_FOR_ID = 335,
    VAR_STATISTICS_CUMULATIVE = 336,
    VAR_OUTGOING_PORT_PERMIT = 337,
    VAR_OUTGOING_PORT_AVOID = 338,
    VAR_DLV_ANCHOR_FILE = 339,
    VAR_DLV_ANCHOR = 340,
    VAR_NEG_CACHE_SIZE = 341,
    VAR_HARDEN_REFERRAL_PATH = 342,
    VAR_PRIVATE_ADDRESS = 343,
    VAR_PRIVATE_DOMAIN = 344,
    VAR_REMOTE_CONTROL = 345,
    VAR_CONTROL_ENABLE = 346,
    VAR_CONTROL_INTERFACE = 347,
    VAR_CONTROL_PORT = 348,
    VAR_SERVER_KEY_FILE = 349,
    VAR_SERVER_CERT_FILE = 350,
    VAR_CONTROL_KEY_FILE = 351,
    VAR_CONTROL_CERT_FILE = 352,
    VAR_CONTROL_USE_CERT = 353,
    VAR_EXTENDED_STATISTICS = 354,
    VAR_LOCAL_DATA_PTR = 355,
    VAR_JOSTLE_TIMEOUT = 356,
    VAR_STUB_PRIME = 357,
    VAR_UNWANTED_REPLY_THRESHOLD = 358,
    VAR_LOG_TIME_ASCII = 359,
    VAR_DOMAIN_INSECURE = 360,
    VAR_PYTHON = 361,
    VAR_PYTHON_SCRIPT = 362,
    VAR_VAL_SIG_SKEW_MIN = 363,
    VAR_VAL_SIG_SKEW_MAX = 364,
    VAR_CACHE_MIN_TTL = 365,
    VAR_VAL_LOG_LEVEL = 366,
    VAR_AUTO_TRUST_ANCHOR_FILE = 367,
    VAR_KEEP_MISSING = 368,
    VAR_ADD_HOLDDOWN = 369,
    VAR_DEL_HOLDDOWN = 370,
    VAR_SO_RCVBUF = 371,
    VAR_EDNS_BUFFER_SIZE = 372,
    VAR_PREFETCH = 373,
    VAR_PREFETCH_KEY = 374,
    VAR_SO_SNDBUF = 375,
    VAR_SO_REUSEPORT = 376,
    VAR_HARDEN_BELOW_NXDOMAIN = 377,
    VAR_IGNORE_CD_FLAG = 378,
    VAR_LOG_QUERIES = 379,
    VAR_TCP_UPSTREAM = 380,
    VAR_SSL_UPSTREAM = 381,
    VAR_SSL_SERVICE_KEY = 382,
    VAR_SSL_SERVICE_PEM = 383,
    VAR_SSL_PORT = 384,
    VAR_FORWARD_FIRST = 385,
    VAR_STUB_FIRST = 386,
    VAR_MINIMAL_RESPONSES = 387,
    VAR_RRSET_ROUNDROBIN = 388,
    VAR_MAX_UDP_SIZE = 389,
    VAR_DELAY_CLOSE = 390,
    VAR_UNBLOCK_LAN_ZONES = 391,
    VAR_INSECURE_LAN_ZONES = 392,
    VAR_INFRA_CACHE_MIN_RTT = 393,
    VAR_DNS64_PREFIX = 394,
    VAR_DNS64_SYNTHALL = 395,
    VAR_DNSTAP = 396,
    VAR_DNSTAP_ENABLE = 397,
    VAR_DNSTAP_SOCKET_PATH = 398,
    VAR_DNSTAP_SEND_IDENTITY = 399,
    VAR_DNSTAP_SEND_VERSION = 400,
    VAR_DNSTAP_IDENTITY = 401,
    VAR_DNSTAP_VERSION = 402,
    VAR_DNSTAP_LOG_RESOLVER_QUERY_MESSAGES = 403,
    VAR_DNSTAP_LOG_RESOLVER_RESPONSE_MESSAGES = 404,
    VAR_DNSTAP_LOG_CLIENT_QUERY_MESSAGES = 405,
    VAR_DNSTAP_LOG_CLIENT_RESPONSE_MESSAGES = 406,
    VAR_DNSTAP_LOG_FORWARDER_QUERY_MESSAGES = 407,
    VAR_DNSTAP_LOG_FORWARDER_RESPONSE_MESSAGES = 408,
    VAR_HARDEN_ALGO_DOWNGRADE = 409,
    VAR_IP_TRANSPARENT = 410,
    VAR_RATELIMIT = 411,
    VAR_RATELIMIT_SLABS = 412,
    VAR_RATELIMIT_SIZE = 413,
    VAR_RATELIMIT_FOR_DOMAIN = 414,
    VAR_RATELIMIT_BELOW_DOMAIN = 415,
    VAR_RATELIMIT_FACTOR = 416,
    VAR_CAPS_WHITELIST = 417,
    VAR_CACHE_MAX_NEGATIVE_TTL = 418,
    VAR_PERMIT_SMALL_HOLDDOWN = 419,
    VAR_QNAME_MINIMISATION = 420
  };
#endif
/* Tokens.  */
#define SPACE 258
#define LETTER 259
#define NEWLINE 260
#define COMMENT 261
#define COLON 262
#define ANY 263
#define ZONESTR 264
#define STRING_ARG 265
#define VAR_SERVER 266
#define VAR_VERBOSITY 267
#define VAR_NUM_THREADS 268
#define VAR_PORT 269
#define VAR_OUTGOING_RANGE 270
#define VAR_INTERFACE 271
#define VAR_DO_IP4 272
#define VAR_DO_IP6 273
#define VAR_DO_UDP 274
#define VAR_DO_TCP 275
#define VAR_TCP_MSS 276
#define VAR_OUTGOING_TCP_MSS 277
#define VAR_CHROOT 278
#define VAR_USERNAME 279
#define VAR_DIRECTORY 280
#define VAR_LOGFILE 281
#define VAR_PIDFILE 282
#define VAR_MSG_CACHE_SIZE 283
#define VAR_MSG_CACHE_SLABS 284
#define VAR_NUM_QUERIES_PER_THREAD 285
#define VAR_RRSET_CACHE_SIZE 286
#define VAR_RRSET_CACHE_SLABS 287
#define VAR_OUTGOING_NUM_TCP 288
#define VAR_INFRA_HOST_TTL 289
#define VAR_INFRA_LAME_TTL 290
#define VAR_INFRA_CACHE_SLABS 291
#define VAR_INFRA_CACHE_NUMHOSTS 292
#define VAR_INFRA_CACHE_LAME_SIZE 293
#define VAR_NAME 294
#define VAR_STUB_ZONE 295
#define VAR_STUB_HOST 296
#define VAR_STUB_ADDR 297
#define VAR_TARGET_FETCH_POLICY 298
#define VAR_HARDEN_SHORT_BUFSIZE 299
#define VAR_HARDEN_LARGE_QUERIES 300
#define VAR_FORWARD_ZONE 301
#define VAR_FORWARD_HOST 302
#define VAR_FORWARD_ADDR 303
#define VAR_DO_NOT_QUERY_ADDRESS 304
#define VAR_HIDE_IDENTITY 305
#define VAR_HIDE_VERSION 306
#define VAR_IDENTITY 307
#define VAR_VERSION 308
#define VAR_HARDEN_GLUE 309
#define VAR_MODULE_CONF 310
#define VAR_TRUST_ANCHOR_FILE 311
#define VAR_TRUST_ANCHOR 312
#define VAR_VAL_OVERRIDE_DATE 313
#define VAR_BOGUS_TTL 314
#define VAR_VAL_CLEAN_ADDITIONAL 315
#define VAR_VAL_PERMISSIVE_MODE 316
#define VAR_INCOMING_NUM_TCP 317
#define VAR_MSG_BUFFER_SIZE 318
#define VAR_KEY_CACHE_SIZE 319
#define VAR_KEY_CACHE_SLABS 320
#define VAR_TRUSTED_KEYS_FILE 321
#define VAR_VAL_NSEC3_KEYSIZE_ITERATIONS 322
#define VAR_USE_SYSLOG 323
#define VAR_OUTGOING_INTERFACE 324
#define VAR_ROOT_HINTS 325
#define VAR_DO_NOT_QUERY_LOCALHOST 326
#define VAR_CACHE_MAX_TTL 327
#define VAR_HARDEN_DNSSEC_STRIPPED 328
#define VAR_ACCESS_CONTROL 329
#define VAR_LOCAL_ZONE 330
#define VAR_LOCAL_DATA 331
#define VAR_INTERFACE_AUTOMATIC 332
#define VAR_STATISTICS_INTERVAL 333
#define VAR_DO_DAEMONIZE 334
#define VAR_USE_CAPS_FOR_ID 335
#define VAR_STATISTICS_CUMULATIVE 336
#define VAR_OUTGOING_PORT_PERMIT 337
#define VAR_OUTGOING_PORT_AVOID 338
#define VAR_DLV_ANCHOR_FILE 339
#define VAR_DLV_ANCHOR 340
#define VAR_NEG_CACHE_SIZE 341
#define VAR_HARDEN_REFERRAL_PATH 342
#define VAR_PRIVATE_ADDRESS 343
#define VAR_PRIVATE_DOMAIN 344
#define VAR_REMOTE_CONTROL 345
#define VAR_CONTROL_ENABLE 346
#define VAR_CONTROL_INTERFACE 347
#define VAR_CONTROL_PORT 348
#define VAR_SERVER_KEY_FILE 349
#define VAR_SERVER_CERT_FILE 350
#define VAR_CONTROL_KEY_FILE 351
#define VAR_CONTROL_CERT_FILE 352
#define VAR_CONTROL_USE_CERT 353
#define VAR_EXTENDED_STATISTICS 354
#define VAR_LOCAL_DATA_PTR 355
#define VAR_JOSTLE_TIMEOUT 356
#define VAR_STUB_PRIME 357
#define VAR_UNWANTED_REPLY_THRESHOLD 358
#define VAR_LOG_TIME_ASCII 359
#define VAR_DOMAIN_INSECURE 360
#define VAR_PYTHON 361
#define VAR_PYTHON_SCRIPT 362
#define VAR_VAL_SIG_SKEW_MIN 363
#define VAR_VAL_SIG_SKEW_MAX 364
#define VAR_CACHE_MIN_TTL 365
#define VAR_VAL_LOG_LEVEL 366
#define VAR_AUTO_TRUST_ANCHOR_FILE 367
#define VAR_KEEP_MISSING 368
#define VAR_ADD_HOLDDOWN 369
#define VAR_DEL_HOLDDOWN 370
#define VAR_SO_RCVBUF 371
#define VAR_EDNS_BUFFER_SIZE 372
#define VAR_PREFETCH 373
#define VAR_PREFETCH_KEY 374
#define VAR_SO_SNDBUF 375
#define VAR_SO_REUSEPORT 376
#define VAR_HARDEN_BELOW_NXDOMAIN 377
#define VAR_IGNORE_CD_FLAG 378
#define VAR_LOG_QUERIES 379
#define VAR_TCP_UPSTREAM 380
#define VAR_SSL_UPSTREAM 381
#define VAR_SSL_SERVICE_KEY 382
#define VAR_SSL_SERVICE_PEM 383
#define VAR_SSL_PORT 384
#define VAR_FORWARD_FIRST 385
#define VAR_STUB_FIRST 386
#define VAR_MINIMAL_RESPONSES 387
#define VAR_RRSET_ROUNDROBIN 388
#define VAR_MAX_UDP_SIZE 389
#define VAR_DELAY_CLOSE 390
#define VAR_UNBLOCK_LAN_ZONES 391
#define VAR_INSECURE_LAN_ZONES 392
#define VAR_INFRA_CACHE_MIN_RTT 393
#define VAR_DNS64_PREFIX 394
#define VAR_DNS64_SYNTHALL 395
#define VAR_DNSTAP 396
#define VAR_DNSTAP_ENABLE 397
#define VAR_DNSTAP_SOCKET_PATH 398
#define VAR_DNSTAP_SEND_IDENTITY 399
#define VAR_DNSTAP_SEND_VERSION 400
#define VAR_DNSTAP_IDENTITY 401
#define VAR_DNSTAP_VERSION 402
#define VAR_DNSTAP_LOG_RESOLVER_QUERY_MESSAGES 403
#define VAR_DNSTAP_LOG_RESOLVER_RESPONSE_MESSAGES 404
#define VAR_DNSTAP_LOG_CLIENT_QUERY_MESSAGES 405
#define VAR_DNSTAP_LOG_CLIENT_RESPONSE_MESSAGES 406
#define VAR_DNSTAP_LOG_FORWARDER_QUERY_MESSAGES 407
#define VAR_DNSTAP_LOG_FORWARDER_RESPONSE_MESSAGES 408
#define VAR_HARDEN_ALGO_DOWNGRADE 409
#define VAR_IP_TRANSPARENT 410
#define VAR_RATELIMIT 411
#define VAR_RATELIMIT_SLABS 412
#define VAR_RATELIMIT_SIZE 413
#define VAR_RATELIMIT_FOR_DOMAIN 414
#define VAR_RATELIMIT_BELOW_DOMAIN 415
#define VAR_RATELIMIT_FACTOR 416
#define VAR_CAPS_WHITELIST 417
#define VAR_CACHE_MAX_NEGATIVE_TTL 418
#define VAR_PERMIT_SMALL_HOLDDOWN 419
#define VAR_QNAME_MINIMISATION 420

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 64 "./util/configparser.y" /* yacc.c:1909  */

	char*	str;

#line 388 "util/configparser.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_UTIL_CONFIGPARSER_H_INCLUDED  */
