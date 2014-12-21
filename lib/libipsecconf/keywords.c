/*
 * Libreswan config file parser (keywords.c)
 * Copyright (C) 2003-2006 Michael Richardson <mcr@xelerance.com>
 * Copyright (C) 2007-2010 Paul Wouters <paul@xelerance.com>
 * Copyright (C) 2012 Paul Wouters <paul@libreswan.org>
 * Copyright (C) 2013 Paul Wouters <pwouters@redhat.com>
 * Copyright (C) 2013 D. Hugh Redelmeier <hugh@mimosa.com>
 * Copyright (C) 2013 David McCullough <ucdevel@gmail.com>
 * Copyright (C) 2013 Antony Antony <antony@phenome.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */

#include <sys/queue.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#ifndef _LIBRESWAN_H
#include <libreswan.h>
#include "constants.h"
#endif

#include "ipsecconf/keywords.h"
#include "ipsecconf/parser.h"	/* includes parser.tab.h generated by bison; requires keywords.h */
#include "ipsecconf/parserlast.h"

#define VALUES_INITIALIZER(t)	{ (t), sizeof(t) / sizeof((t)[0]) }

/*
 * Values for failureshunt={passthrough, drop, reject, none}
 */
static const struct keyword_enum_value kw_failureshunt_values[] = {
	{ "none",        POLICY_FAIL_NONE },
	{ "passthrough", POLICY_FAIL_PASS },
	{ "drop",        POLICY_FAIL_DROP },
	{ "reject",      POLICY_FAIL_REJECT },
};

static const struct keyword_enum_values kw_failureshunt_list = VALUES_INITIALIZER(kw_failureshunt_values);

/*
 * Values for keyexchange=
 */
static const struct keyword_enum_value kw_keyexchange_values[] = {
	{ "ike",  KE_IKE },
};

static const struct keyword_enum_values kw_keyexchange_list = VALUES_INITIALIZER(kw_keyexchange_values);

/*
 * Values for Four-State options, such as ikev2
 */
static const struct keyword_enum_value kw_fourvalued_values[] = {
	{ "never",     fo_never  },
	{ "permit",    fo_permit },
	{ "propose",   fo_propose },
	{ "insist",    fo_insist },
	{ "yes",       fo_propose },
	{ "always",    fo_insist },
	{ "no",        fo_never  }
};

static const struct keyword_enum_values kw_fourvalued_list = VALUES_INITIALIZER(kw_fourvalued_values);

/*
 * Values for yes/no/force, used by ike_frag
 */
static const struct keyword_enum_value kw_ynf_values[] = {
	{ "never",     ynf_no },
	{ "no",        ynf_no },
	{ "yes",       ynf_yes },
	{ "insist",     ynf_force },
	{ "force",     ynf_force },
};

static const struct keyword_enum_values kw_ynf_list = VALUES_INITIALIZER(kw_ynf_values);

/*
 * Values for authby={rsasig, secret}
 */
static const struct keyword_enum_value kw_authby_values[] = {
	{ "never",     LEMPTY },
	{ "secret",    POLICY_PSK },
	{ "rsasig",    POLICY_RSASIG },
	{ "secret|rsasig",    POLICY_PSK | POLICY_RSASIG},
};

static const struct keyword_enum_values kw_authby_list = VALUES_INITIALIZER(kw_authby_values);

/*
 * Values for dpdaction={hold,clear,restart}
 */
static const struct keyword_enum_value kw_dpdaction_values[] = {
	{ "hold",    DPD_ACTION_HOLD },
	{ "clear",   DPD_ACTION_CLEAR },
	{ "restart",   DPD_ACTION_RESTART },
	/* obsoleted keyword - functionality moved into "restart" */
	{ "restart_by_peer",   DPD_ACTION_RESTART },
};

static const struct keyword_enum_values kw_dpdaction_list = VALUES_INITIALIZER(kw_dpdaction_values);

/*
 * Values for sendca={none,issuer,all}
 */

static const struct keyword_enum_value kw_sendca_values[] = {
	{ "none",	CA_SEND_NONE },
	{ "issuer",	CA_SEND_ISSUER },
	{ "all",	CA_SEND_ALL },
};

static const struct keyword_enum_values kw_sendca_list = VALUES_INITIALIZER(kw_sendca_values);

/*
 * Values for auto={add,start,route,ignore}
 */
static const struct keyword_enum_value kw_auto_values[] = {
	{ "ignore", STARTUP_IGNORE },
	/* no keyword for STARTUP_POLICY */
	{ "add",    STARTUP_ADD },
	{ "ondemand",  STARTUP_ONDEMAND },
	{ "route",  STARTUP_ONDEMAND }, /* backwards compatibility alias */
	{ "start",  STARTUP_START },
	{ "up",     STARTUP_START }, /* alias */
};

static const struct keyword_enum_values kw_auto_list = VALUES_INITIALIZER(kw_auto_values);

/*
 * Values for connaddrfamily={ipv4,ipv6}
 */
static const struct keyword_enum_value kw_connaddrfamily_values[] = {
	{ "ipv4",   AF_INET },
	{ "ipv6",   AF_INET6 },
};

static const struct keyword_enum_values kw_connaddrfamily_list = VALUES_INITIALIZER(kw_connaddrfamily_values);

/*
 * Values for type={tunnel,transport,udpencap}
 */
static const struct keyword_enum_value kw_type_values[] = {
	{ "tunnel",    KS_TUNNEL },
	{ "transport", KS_TRANSPORT },
	{ "pass",      KS_PASSTHROUGH },
	{ "passthrough", KS_PASSTHROUGH },
	{ "reject",    KS_REJECT },
	{ "drop",      KS_DROP },
};

static const struct keyword_enum_values kw_type_list = VALUES_INITIALIZER(kw_type_values);

/*
 * Values for rsasigkey={%dnsondemand, %dns, literal }
 */
static const struct keyword_enum_value kw_rsasigkey_values[] = {
	{ "",             PUBKEY_PREEXCHANGED },
	{ "%cert",        PUBKEY_CERTIFICATE },
	{ "%dns",         PUBKEY_DNS },
	{ "%dnsondemand", PUBKEY_DNSONDEMAND },
};

static const struct keyword_enum_values kw_rsasigkey_list = VALUES_INITIALIZER(kw_rsasigkey_values);

/*
 * Values for protostack={netkey, klips, mast or none }
 */
static const struct keyword_enum_value kw_proto_stack_list[] = {
	{ "none",         NO_KERNEL },
	{ "klips",        USE_KLIPS },
	{ "mast",         USE_MASTKLIPS },
	{ "auto",         USE_NETKEY }, /* auto now means netkey */
	{ "netkey",       USE_NETKEY },
	{ "native",       USE_NETKEY },
	{ "bsd",          USE_BSDKAME },
	{ "kame",         USE_BSDKAME },
	{ "bsdkame",      USE_BSDKAME },
	{ "win2k",        USE_WIN2K },
};

static const struct keyword_enum_values kw_proto_stack = VALUES_INITIALIZER(kw_proto_stack_list);

/*
 * Values for sareftrack={yes, no, conntrack }
 */
static const struct keyword_enum_value kw_sareftrack_values[] = {
	{ "yes",          sat_yes },
	{ "no",           sat_no },
	{ "conntrack",    sat_conntrack },
};

static const struct keyword_enum_values kw_sareftrack_list = VALUES_INITIALIZER(kw_sareftrack_values);

/*
 *  Cisco interop: remote peer type
 */

static const struct keyword_enum_value kw_remote_peer_type_list[] = {
	{ "cisco",         CISCO },
};

static const struct keyword_enum_values kw_remote_peer_type = VALUES_INITIALIZER(kw_remote_peer_type_list);

static const struct keyword_enum_value kw_xauthby_list[] = {
	{ "file",	XAUTHBY_FILE },
	{ "pam",	XAUTHBY_PAM },
	{ "alwaysok",	XAUTHBY_ALWAYSOK },
};

static const struct keyword_enum_values kw_xauthby = VALUES_INITIALIZER(kw_xauthby_list);

static const struct keyword_enum_value kw_xauthfail_list[] = {
	{ "hard",         XAUTHFAIL_HARD },
	{ "soft",         XAUTHFAIL_SOFT },
};

static const struct keyword_enum_values kw_xauthfail = VALUES_INITIALIZER(kw_xauthfail_list);

/*
 * Values for right= and left=
 */

static const struct keyword_enum_value kw_plutodebug_values[] = {
	{ "none",     DBG_NONE },
	{ "all",      DBG_ALL },
	{ "raw",      DBG_RAW },
	{ "crypt",    DBG_CRYPT },
	{ "parsing",  DBG_PARSING },
	{ "emitting", DBG_EMITTING },
	{ "control",  DBG_CONTROL },
	{ "lifecycle", DBG_LIFECYCLE },
	{ "kernel",    DBG_KERNEL },
	{ "dns",      DBG_DNS },
	{ "oppo",     DBG_OPPO },
	{ "oppoinfo",    DBG_OPPOINFO },
	{ "controlmore", DBG_CONTROLMORE },
	{ "private",  DBG_PRIVATE },
	{ "x509",     DBG_X509 },
	{ "dpd",      DBG_DPD },
	{ "pfkey",    DBG_PFKEY },
	{ "natt",     DBG_NATT },
	{ "nattraversal", DBG_NATT },
	/* backwards compatibility */
	{ "klips",    DBG_KERNEL },
	{ "netkey",    DBG_KERNEL },

	{ "impair-delay-adns-key-answer", IMPAIR_DELAY_ADNS_KEY_ANSWER },
	{ "impair-delay-adns-txt-answer", IMPAIR_DELAY_ADNS_TXT_ANSWER },
	{ "impair-bust-mi2", IMPAIR_BUST_MI2 },
	{ "impair-bust-mr2", IMPAIR_BUST_MR2 },
};

static const struct keyword_enum_values kw_plutodebug_list = VALUES_INITIALIZER(kw_plutodebug_values);

static const struct keyword_enum_value kw_klipsdebug_values[] = {
	{ "all",      LRANGE(KDF_XMIT, KDF_COMP) },
	{ "none",     0 },
	{ "verbose",  LELEM(KDF_VERBOSE) },
	{ "xmit",     LELEM(KDF_XMIT) },
	{ "tunnel-xmit", LELEM(KDF_XMIT) },
	{ "netlink",  LELEM(KDF_NETLINK) },
	{ "xform",    LELEM(KDF_XFORM) },
	{ "eroute",   LELEM(KDF_EROUTE) },
	{ "spi",      LELEM(KDF_SPI) },
	{ "radij",    LELEM(KDF_RADIJ) },
	{ "esp",      LELEM(KDF_ESP) },
	{ "ah",       LELEM(KDF_AH) },
	{ "rcv",      LELEM(KDF_RCV) },
	{ "tunnel",   LELEM(KDF_TUNNEL) },
	{ "pfkey",    LELEM(KDF_PFKEY) },
	{ "comp",     LELEM(KDF_COMP) },
	{ "nat-traversal", LELEM(KDF_NATT) },
	{ "nattraversal", LELEM(KDF_NATT) },
	{ "natt",     LELEM(KDF_NATT) },
};

static const struct keyword_enum_values kw_klipsdebug_list = VALUES_INITIALIZER(kw_klipsdebug_values);

static const struct keyword_enum_value kw_phase2types_values[] = {
	{ "ah+esp",   POLICY_ENCRYPT | POLICY_AUTHENTICATE },
	{ "esp",      POLICY_ENCRYPT },
	{ "ah",       POLICY_AUTHENTICATE },
	{ "default",  POLICY_ENCRYPT }, /* alias, find it last */
};

static const struct keyword_enum_values kw_phase2types_list = VALUES_INITIALIZER(kw_phase2types_values);

/*
 * Values for {left/right}sendcert={never,sendifasked,always,forcedtype}
 */
static const struct keyword_enum_value kw_sendcert_values[] = {
	{ "never",        cert_neversend },
	{ "sendifasked",  cert_sendifasked },
	{ "alwayssend",   cert_alwayssend },
	{ "always",       cert_alwayssend },
};

static const struct keyword_enum_values kw_sendcert_list = VALUES_INITIALIZER(kw_sendcert_values);

/*
 * Values for nat-ikev1-method={drafts,rfc,both}
 */
static const struct keyword_enum_value kw_ikev1natt_values[] = {
	{ "both",       natt_both },
	{ "rfc",        natt_rfc },
	{ "drafts",     natt_drafts },
};

static const struct keyword_enum_values kw_ikev1natt_list = VALUES_INITIALIZER(kw_ikev1natt_values);

/* MASTER KEYWORD LIST
 * Note: this table is terminated by an entry with keyname == NULL.
 */
const struct keyword_def ipsec_conf_keywords_v2[] = {
	{ "interfaces",     kv_config, kt_string,    KSF_INTERFACES,
	  NOT_ENUM },
	{ "myid",           kv_config, kt_string,    KSF_MYID, NOT_ENUM },
	{ "myvendorid",     kv_config, kt_string,    KSF_MYVENDORID, NOT_ENUM },
	{ "syslog",         kv_config, kt_string,    KSF_SYSLOG, NOT_ENUM },
	{ "klipsdebug",     kv_config, kt_list,      KBF_KLIPSDEBUG,
	  &kw_klipsdebug_list },
	{ "plutodebug",     kv_config, kt_list,      KBF_PLUTODEBUG,
	  &kw_plutodebug_list },
	{ "plutostderrlog", kv_config, kt_filename,  KSF_PLUTOSTDERRLOG,
	  NOT_ENUM },
	{ "plutostderrlogtime",        kv_config, kt_bool,
	  KBF_PLUTOSTDERRLOGTIME, NOT_ENUM },
	{ "plutorestartoncrash", kv_config, kt_bool, KBF_PLUTORESTARTONCRASH,
	  NOT_ENUM },
	{ "dumpdir",        kv_config, kt_dirname,   KSF_DUMPDIR, NOT_ENUM },
	{ "ipsecdir",        kv_config, kt_dirname,   KSF_IPSECDIR, NOT_ENUM },
	{ "secretsfile",        kv_config, kt_dirname,   KSF_SECRETSFILE,
	  NOT_ENUM },
	{ "statsbin",        kv_config, kt_dirname,   KSF_STATSBINARY, NOT_ENUM },
	{ "plutofork",           kv_config, kt_bool,      KBF_PLUTOFORK,
	  NOT_ENUM },
	{ "perpeerlog",        kv_config, kt_bool,   KBF_PERPEERLOG,
	  NOT_ENUM },
	{ "perpeerlogdir",        kv_config, kt_dirname,   KSF_PERPEERDIR,
	  NOT_ENUM },
	{ "oe",             kv_config, kt_obsolete_quiet,      KBF_WARNIGNORE,
	  NOT_ENUM },
	{ "fragicmp",       kv_config, kt_bool,      KBF_FRAGICMP, NOT_ENUM },
	{ "hidetos",        kv_config, kt_bool,      KBF_HIDETOS, NOT_ENUM },
	{ "uniqueids",      kv_config, kt_bool,      KBF_UNIQUEIDS, NOT_ENUM },
	{ "overridemtu",    kv_config, kt_number,    KBF_OVERRIDEMTU,
	  NOT_ENUM },
	{ "strictcrlpolicy", kv_config, kt_bool,      KBF_STRICTCRLPOLICY,
	  NOT_ENUM },
	{ "crlcheckinterval", kv_config, kt_time,     KBF_CRLCHECKINTERVAL,
	  NOT_ENUM },
	{ "force_busy",     kv_config | kv_alias, kt_bool,      KBF_FORCEBUSY, NOT_ENUM },	/* obsolete _ */
	{ "force-busy",     kv_config, kt_bool,      KBF_FORCEBUSY, NOT_ENUM },
	{ "ikeport",        kv_config, kt_number,     KBF_IKEPORT, NOT_ENUM },

	{ "virtual_private", kv_config | kv_alias, kt_string,     KSF_VIRTUALPRIVATE, NOT_ENUM },	/* obsolete _ */
	{ "virtual-private", kv_config, kt_string,     KSF_VIRTUALPRIVATE, NOT_ENUM },
	{ "nat_ikeport",   kv_config | kv_alias, kt_number,      KBF_NATIKEPORT, NOT_ENUM },	/* obsolete _ */
	{ "nat-ikeport",   kv_config, kt_number,      KBF_NATIKEPORT, NOT_ENUM },
	{ "keep_alive", kv_config | kv_alias, kt_number,    KBF_KEEPALIVE, NOT_ENUM },	/* obsolete _ */
	{ "keep-alive", kv_config, kt_number,    KBF_KEEPALIVE, NOT_ENUM },
	{ "nat_traversal", kv_config, kt_obsolete_quiet, KBF_WARNIGNORE, NOT_ENUM },
	{ "disable_port_floating", kv_config, kt_obsolete, KBF_WARNIGNORE, NOT_ENUM },
	{ "force_keepalive", kv_config, kt_obsolete,    KBF_WARNIGNORE,
	  NOT_ENUM },

	{ "listen",     kv_config, kt_string, KSF_LISTEN, NOT_ENUM },
	{ "protostack",     kv_config, kt_string,    KSF_PROTOSTACK,
	  &kw_proto_stack },
	{ "nhelpers", kv_config, kt_number, KBF_NHELPERS, NOT_ENUM },
#ifdef HAVE_LABELED_IPSEC
	/* ??? AN ATTRIBUTE TYPE, NOT VALUE! */
	{ "secctx_attr_value", kv_config | kv_alias, kt_number, KBF_SECCTX, NOT_ENUM },	/* obsolete _ */
	{ "secctx-attr-value", kv_config, kt_number, KBF_SECCTX, NOT_ENUM },	/* obsolete: not a value, a type */
	{ "secctx-attr-type", kv_config, kt_number, KBF_SECCTX, NOT_ENUM },
#endif
	/* these options are obsoleted. Don't die on them */
	{ "forwardcontrol", kv_config, kt_obsolete, KBF_WARNIGNORE, NOT_ENUM },
	{ "rp_filter",      kv_config, kt_obsolete, KBF_WARNIGNORE, NOT_ENUM },
	{ "pluto",      kv_config, kt_obsolete, KBF_WARNIGNORE, NOT_ENUM },
	{ "prepluto",      kv_config, kt_obsolete, KBF_WARNIGNORE, NOT_ENUM },
	{ "postpluto",      kv_config, kt_obsolete, KBF_WARNIGNORE, NOT_ENUM },
	{ "plutoopts",      kv_config, kt_obsolete, KBF_WARNIGNORE, NOT_ENUM },
	{ "plutowait",      kv_config, kt_obsolete, KBF_WARNIGNORE, NOT_ENUM },
	{ "nocrsend",       kv_config, kt_obsolete, KBF_WARNIGNORE, NOT_ENUM },

	/* this is "left=" and "right=" */
	{ "",               kv_conn | kv_leftright, kt_loose_enum, KSCF_IP,
	  &kw_host_list },

	{ "ike",            kv_conn | kv_auto, kt_string, KSF_IKE, NOT_ENUM },

	{ "subnet",         kv_conn | kv_auto | kv_leftright | kv_processed,
	  kt_subnet, KSCF_SUBNET, NOT_ENUM },
	{ "subnets",        kv_conn | kv_auto | kv_leftright, kt_appendlist,
	  KSCF_SUBNETS, NOT_ENUM },
	{ "sourceip",       kv_conn | kv_auto | kv_leftright, kt_ipaddr,
	  KSCF_SOURCEIP, NOT_ENUM },
	{ "nexthop",        kv_conn | kv_auto | kv_leftright, kt_ipaddr,
	  KSCF_NEXTHOP, NOT_ENUM },
	{ "updown",         kv_conn | kv_auto | kv_leftright, kt_filename,
	  KSCF_UPDOWN, NOT_ENUM },
	{ "id",             kv_conn | kv_auto | kv_leftright, kt_idtype,
	  KSCF_ID, NOT_ENUM },
	{ "rsasigkey",      kv_conn | kv_auto | kv_leftright, kt_rsakey,
	  KSCF_RSAKEY1, &kw_rsasigkey_list },
	{ "rsasigkey2",     kv_conn | kv_auto | kv_leftright, kt_rsakey,
	  KSCF_RSAKEY2, &kw_rsasigkey_list },
	{ "cert",           kv_conn | kv_auto | kv_leftright, kt_filename,
	  KSCF_CERT, NOT_ENUM },
	{ "sendcert",       kv_conn | kv_auto | kv_leftright, kt_enum,
	  KNCF_SENDCERT, &kw_sendcert_list },
	{ "ca",             kv_conn | kv_auto | kv_leftright, kt_string,
	  KSCF_CA, NOT_ENUM },

	/* these are conn statements which are not left/right */
	{ "auto",           kv_conn | kv_duplicateok, kt_enum,   KBF_AUTO,
	  &kw_auto_list },
	{ "also",           kv_conn,         kt_appendstring, KSF_ALSO,
	  NOT_ENUM },
	{ "alsoflip",       kv_conn,         kt_string, KSF_ALSOFLIP,
	  NOT_ENUM },
	{ "connaddrfamily", kv_conn,         kt_enum,   KBF_CONNADDRFAMILY,
	  &kw_connaddrfamily_list },
	{ "type",           kv_conn,         kt_enum,   KBF_TYPE,
	  &kw_type_list },
	{ "authby",         kv_conn | kv_auto, kt_enum,   KBF_AUTHBY,
	  &kw_authby_list },
	{ "keyexchange",    kv_conn | kv_auto, kt_enum,   KBF_KEYEXCHANGE,
	  &kw_keyexchange_list },
	{ "ikev2",          kv_conn | kv_auto | kv_processed, kt_enum,
	  KBF_IKEv2, &kw_fourvalued_list },
	{ "ike_frag",       kv_conn | kv_auto | kv_processed | kv_alias, kt_enum,
	  KBF_IKE_FRAG, &kw_ynf_list },	/* obsolete _ */
	{ "ike-frag",       kv_conn | kv_auto | kv_processed, kt_enum,
	  KBF_IKE_FRAG, &kw_ynf_list },
	{ "narrowing",      kv_conn | kv_auto, kt_bool,
	  KBF_IKEv2_ALLOW_NARROWING, NOT_ENUM },
	{ "sareftrack",     kv_conn | kv_auto | kv_processed, kt_enum,
	  KBF_SAREFTRACK, &kw_sareftrack_list },
	{ "pfs",            kv_conn | kv_auto, kt_bool,   KBF_PFS,
	  NOT_ENUM },

	{ "nat_keepalive",  kv_conn | kv_auto | kv_alias, kt_bool,   KBF_NAT_KEEPALIVE,
	  NOT_ENUM },	/* obsolete _ */
	{ "nat-keepalive",  kv_conn | kv_auto, kt_bool,   KBF_NAT_KEEPALIVE,
	  NOT_ENUM },

	{ "initial_contact", kv_conn | kv_auto | kv_alias, kt_bool,   KBF_INITIAL_CONTACT,
	  NOT_ENUM },	/* obsolete _ */
	{ "initial-contact", kv_conn | kv_auto, kt_bool,   KBF_INITIAL_CONTACT,
	  NOT_ENUM },
	{ "cisco_unity", kv_conn | kv_auto | kv_alias, kt_bool,   KBF_CISCO_UNITY,
	  NOT_ENUM },	/* obsolete _ */
	{ "cisco-unity", kv_conn | kv_auto, kt_bool,   KBF_CISCO_UNITY,
	  NOT_ENUM },
	{ "send_vendorid", kv_conn | kv_auto | kv_alias, kt_bool,   KBF_SEND_VENDORID,
	  NOT_ENUM },	/* obsolete _ */
	{ "send-vendorid", kv_conn | kv_auto, kt_bool,   KBF_SEND_VENDORID,
	  NOT_ENUM },
	{ "sha2_truncbug",  kv_conn | kv_auto | kv_alias, kt_bool,   KBF_SHA2_TRUNCBUG,
	  NOT_ENUM },	/* obsolete _ */
	{ "sha2-truncbug",  kv_conn | kv_auto, kt_bool,   KBF_SHA2_TRUNCBUG,
	  NOT_ENUM },
	{ "keylife",        kv_conn | kv_auto | kv_alias, kt_time,
	  KBF_SALIFETIME, NOT_ENUM },
	{ "lifetime",       kv_conn | kv_auto | kv_alias, kt_time,
	  KBF_SALIFETIME, NOT_ENUM },
	{ "salifetime",     kv_conn | kv_auto, kt_time,   KBF_SALIFETIME,
	  NOT_ENUM },
	{"ikepad",          kv_conn | kv_auto, kt_bool,   KBF_IKEPAD,
	  NOT_ENUM },
	{ "nat-ikev1-method", kv_conn | kv_auto | kv_processed, kt_enum,
	  KBF_IKEV1_NATT, &kw_ikev1natt_list },
#ifdef HAVE_LABELED_IPSEC
	{ "loopback",       kv_conn | kv_auto, kt_bool, KBF_LOOPBACK,
	  NOT_ENUM },
	{ "labeled_ipsec",   kv_conn | kv_auto | kv_alias, kt_bool, KBF_LABELED_IPSEC,
	  NOT_ENUM },	/* obsolete _ */
	{ "labeled-ipsec",   kv_conn | kv_auto, kt_bool, KBF_LABELED_IPSEC,
	  NOT_ENUM },
	{ "policy_label",    kv_conn | kv_auto | kv_alias,         kt_string,
	  KSF_POLICY_LABEL, NOT_ENUM },	/* obsolete _ */
	{ "policy-label",    kv_conn | kv_auto,         kt_string,
	  KSF_POLICY_LABEL, NOT_ENUM },
#endif

	/* Cisco interop: remote peer type */
	{ "remote_peer_type", kv_conn | kv_auto | kv_alias, kt_enum, KBF_REMOTEPEERTYPE,
	  &kw_remote_peer_type },	/* obsolete _ */
	{ "remote-peer-type", kv_conn | kv_auto, kt_enum, KBF_REMOTEPEERTYPE,
	  &kw_remote_peer_type },

	/* Network Manager support */
#ifdef HAVE_NM
	{ "nm_configured", kv_conn | kv_auto | kv_alias, kt_bool, KBF_NMCONFIGURED,
	  NOT_ENUM },	/* obsolete _ */
	{ "nm-configured", kv_conn | kv_auto, kt_bool, KBF_NMCONFIGURED,
	  NOT_ENUM },
#endif

	{ "xauthby", kv_conn | kv_auto, kt_enum, KBF_XAUTHBY, &kw_xauthby },
	{ "xauthfail", kv_conn | kv_auto, kt_enum, KBF_XAUTHFAIL,
	  &kw_xauthfail },
	{ "xauthserver", kv_conn | kv_auto | kv_leftright, kt_bool,
	  KNCF_XAUTHSERVER,  NOT_ENUM },
	{ "xauthclient", kv_conn | kv_auto | kv_leftright, kt_bool,
	  KNCF_XAUTHCLIENT, NOT_ENUM },
	{ "xauthname",   kv_conn | kv_auto | kv_leftright, kt_string,
	  KSCF_XAUTHUSERNAME, NOT_ENUM },
	{ "modecfgserver", kv_conn | kv_auto | kv_leftright, kt_bool,
	  KNCF_MODECONFIGSERVER, NOT_ENUM },
	{ "modecfgclient", kv_conn | kv_auto | kv_leftright, kt_bool,
	  KNCF_MODECONFIGCLIENT, NOT_ENUM },
	{ "xauthusername", kv_conn | kv_auto | kv_leftright, kt_string,
	  KSCF_XAUTHUSERNAME, NOT_ENUM },
	{ "modecfgpull", kv_conn | kv_auto, kt_invertbool, KBF_MODECONFIGPULL,
	  NOT_ENUM },
	/* these are really kt_ipaddr, but we handle them as string until we load them into a whack message */
	{ "modecfgdns1", kv_conn | kv_auto, kt_string, KSF_MODECFGDNS1,
	  NOT_ENUM },
	{ "modecfgdns2", kv_conn | kv_auto, kt_string, KSF_MODECFGDNS2,
	  NOT_ENUM },

	{ "modecfgdomain", kv_conn | kv_auto, kt_string, KSF_MODECFGDOMAIN,
	  NOT_ENUM },
	{ "modecfgbanner", kv_conn | kv_auto, kt_string, KSF_MODECFGBANNER,
	  NOT_ENUM },
	{ "addresspool", kv_conn | kv_auto | kv_leftright, kt_range,
	  KSCF_ADDRESSPOOL, NOT_ENUM },
	{ "modecfgwins1", kv_conn | kv_auto, kt_obsolete, KBF_WARNIGNORE,
	  NOT_ENUM },
	{ "modecfgwins2", kv_conn | kv_auto, kt_obsolete, KBF_WARNIGNORE,
	  NOT_ENUM },

	{ "forceencaps",    kv_conn | kv_auto, kt_bool,   KBF_FORCEENCAP,
	  NOT_ENUM },

	{ "overlapip",      kv_conn | kv_auto, kt_bool,   KBF_OVERLAPIP,
	  NOT_ENUM },
	{ "rekey",          kv_conn | kv_auto, kt_bool,   KBF_REKEY,
	  NOT_ENUM },
	{ "rekeymargin",    kv_conn | kv_auto, kt_time,   KBF_REKEYMARGIN,
	  NOT_ENUM },
	{ "rekeyfuzz",      kv_conn | kv_auto, kt_percent,   KBF_REKEYFUZZ,
	  NOT_ENUM },
	{ "keyingtries",    kv_conn | kv_auto, kt_number, KBF_KEYINGTRIES,
	  NOT_ENUM },
	{ "ikelifetime",    kv_conn | kv_auto, kt_time,   KBF_IKELIFETIME,
	  NOT_ENUM },
	{ "disablearrivalcheck", kv_conn | kv_auto, kt_invertbool,
	  KBF_ARRIVALCHECK, NOT_ENUM },
	{ "failureshunt",   kv_conn | kv_auto, kt_enum,   KBF_FAILURESHUNT,
	  &kw_failureshunt_list },
	{ "connalias",      kv_conn | kv_processed | kv_auto | kv_manual,
	  kt_appendstring,   KSF_CONNALIAS, NOT_ENUM },

	/* attributes of the phase2 policy */
	{ "phase2alg",      kv_conn | kv_auto | kv_manual,  kt_string, KSF_ESP,
	  NOT_ENUM },
	{ "esp",            kv_conn | kv_auto | kv_manual | kv_alias,
	  kt_string, KSF_ESP, NOT_ENUM },
	{ "ah",             kv_conn | kv_auto | kv_manual | kv_alias,
	  kt_string, KSF_ESP, NOT_ENUM },
	{ "subnetwithin",   kv_conn | kv_leftright, kt_string,
	  KSCF_SUBNETWITHIN, NOT_ENUM },
	{ "protoport",      kv_conn | kv_leftright | kv_processed, kt_string,
	  KSCF_PROTOPORT, NOT_ENUM },
	{ "phase2",         kv_conn | kv_auto | kv_manual | kv_policy,
	  kt_enum, KBF_PHASE2, &kw_phase2types_list },
	{ "auth",           kv_conn | kv_auto | kv_manual | kv_policy |
	  kv_alias,  kt_enum, KBF_PHASE2, &kw_phase2types_list },
	{ "compress",       kv_conn | kv_auto, kt_bool,   KBF_COMPRESS,
	  NOT_ENUM },

	/* route metric */
	{ "metric",         kv_conn | kv_auto, kt_number, KBF_METRIC,
	  NOT_ENUM },

	/* DPD */
	{ "dpddelay",       kv_conn | kv_auto, kt_number, KBF_DPDDELAY,
	  NOT_ENUM },
	{ "dpdtimeout",     kv_conn | kv_auto, kt_number, KBF_DPDTIMEOUT,
	  NOT_ENUM },
	{ "dpdaction",      kv_conn | kv_auto, kt_enum, KBF_DPDACTION,
	  &kw_dpdaction_list },

	{ "sendca",	    kv_conn | kv_auto, kt_enum, KBF_SEND_CA,
	  &kw_sendca_list },

	{ "mtu",            kv_conn | kv_auto, kt_number, KBF_CONNMTU,
	  NOT_ENUM },
	{ "priority",       kv_conn | kv_auto, kt_number, KBF_PRIORITY,
	  NOT_ENUM },
	{ "reqid",          kv_conn | kv_auto, kt_number, KBF_REQID,
	  NOT_ENUM },

	{ "aggrmode",    kv_conn | kv_auto, kt_invertbool,      KBF_AGGRMODE,
	  NOT_ENUM },

	{ NULL, 0, 0, 0, NOT_ENUM }
};

/* distinguished keyword */
static const struct keyword_def ipsec_conf_keyword_comment =
	{ "x-comment",      kv_conn,   kt_comment, 0, NOT_ENUM };


/*
 * look for one of the above tokens, and set the value up right.
 *
 * if we don't find it, then strdup() the string and return a string
 *
 */

/* type is really "token" type, which is actually int */
int parser_find_keyword(const char *s, YYSTYPE *lval)
{
	const struct keyword_def *k;
	bool keyleft;
	int keywordtype;

	keyleft = FALSE;
	k = ipsec_conf_keywords_v2;

	while (k->keyname != NULL) {
		if (strcaseeq(s, k->keyname))
			break;

		if (k->validity & kv_leftright) {
			if (strncaseeq(s, "left", 4) &&
			    strcaseeq(s + 4, k->keyname)) {
				keyleft = TRUE;
				break;
			} else if (strncaseeq(s, "right", 5) &&
				   strcaseeq(s + 5, k->keyname)) {
				keyleft = FALSE;
				break;
			}
		}

		k++;
	}

	lval->s = NULL;
	/* if we found nothing */
	if (k->keyname == NULL &&
	    (s[0] == 'x' || s[0] == 'X') && (s[1] == '-' || s[1] == '_')) {
		k = &ipsec_conf_keyword_comment;
		lval->k.string = strdup(s);
	}

	/* if we still found nothing */
	if (k->keyname == NULL) {
		lval->s = strdup(s);
		return STRING;
	}

	switch (k->type) {
	case kt_percent:
		keywordtype = PERCENTWORD;
		break;
	case kt_time:
		keywordtype = TIMEWORD;
		break;
	case kt_comment:
		keywordtype = COMMENT;
		break;
	case kt_bool:
	case kt_invertbool:
		keywordtype = BOOLWORD;
		break;
	default:
		keywordtype = KEYWORD;
		break;
	}

	/* else, set up llval.k to point, and return KEYWORD */
	lval->k.keydef = k;
	lval->k.keyleft = keyleft;
	return keywordtype;
}

unsigned int parser_enum_list(const struct keyword_def *kd, const char *s, bool list)
{
	char *piece;
	char *scopy;
	int numfound, kevcount;
	const struct keyword_enum_value *kev;
	unsigned int valresult;
	char complaintbuf[80];

	assert(kd->type == kt_list || kd->type == kt_enum);

	scopy = strdup(s);
	valresult = 0;

	/*
	 * split up the string into comma separated pieces, and look each piece up in the
	 * value list provided in the definition.
	 */

	numfound = 0;
	while ((piece = strsep(&scopy, ":, \t")) != NULL) {
		/* discard empty strings */
		if (strlen(piece) == 0)
			continue;

		assert(kd->validenum != NULL);
		for (kevcount = kd->validenum->valuesize,
		     kev = kd->validenum->values;
		     kevcount > 0 && !strcaseeq(piece, kev->name);
		     kev++, kevcount--)
			;

		/* if we found something */
		if (kevcount != 0) {
			/* count it */
			numfound++;

			valresult |= kev->value;
		} else { /* we didn't find anything, complain */

			snprintf(complaintbuf, sizeof(complaintbuf),
				 "%s: %d: keyword %s, invalid value: %s",
				 parser_cur_filename(), parser_cur_lineno(),
				 kd->keyname, piece);

				fprintf(stderr, "ERROR: %s\n", complaintbuf);
				exit(1);
		}
	}

	if (numfound > 1 && !list) {
		snprintf(complaintbuf, sizeof(complaintbuf),
			 "%s: %d: keyword %s accepts only one value, not %s",
			 parser_cur_filename(), parser_cur_lineno(),
			 kd->keyname, scopy);

			fprintf(stderr, "ERROR: %s\n", complaintbuf);
			free(scopy);
			exit(1);
	}

	free(scopy);
	return valresult;
}

unsigned int parser_loose_enum(struct keyword *k, const char *s)
{
	const struct keyword_def *kd = k->keydef;
	int kevcount;
	const struct keyword_enum_value *kev;
	unsigned int valresult;

	assert(kd->type == kt_loose_enum || kd->type == kt_rsakey);
	assert(kd->validenum != NULL && kd->validenum->values != NULL);

	for (kevcount = kd->validenum->valuesize, kev = kd->validenum->values;
	     kevcount > 0 && !strcaseeq(s, kev->name);
	     kev++, kevcount--)
		;

	/* if we found something */
	if (kevcount != 0) {
		assert(kev->value != 0);
		valresult = kev->value;
		k->string = NULL;
		return valresult;
	}

	k->string = strdup(s);	/* ??? why not xstrdup? */
	return 255;
}
