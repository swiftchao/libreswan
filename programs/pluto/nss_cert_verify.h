/* NSS certificate verification routines for libreswan
 *
 * Copyright (C) 2015 Matt Rogers <mrogers@libreswan.org>
 * Copyright (C) 2018 Andrew Cagney
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <https://www.gnu.org/licenses/gpl2.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */
#ifndef NSS_CERT_VFY_H
#define NSS_CERT_VFY_H

#include <cert.h>

#include "defs.h"
#include "chunk.h"

struct certs;
struct payload_digest;

/* rev_opts index */
struct rev_opts {
	bool ocsp;
	bool ocsp_strict;
	bool ocsp_post;
	bool crl_strict;
};

extern int verify_and_cache_chain(enum ike_version ike_version,
				  struct payload_digest *cert_payloads,
				  struct certs **certs_out,
				  const struct rev_opts *rev_opts);

extern bool cert_VerifySubjectAltName(const CERTCertificate *cert, const char *name);

#define VERIFY_RET_OK       0x0001
#define VERIFY_RET_REVOKED  0x0002
#define VERIFY_RET_FAIL     0x0004
#define VERIFY_RET_SKIP     0x0008

#define VERIFY_RET_CRL_NEED 0x1000

extern SECItem *nss_pkcs7_blob(CERTCertificate *cert, bool send_full_chain);

#endif /* NSS_CERT_VFY_H */
