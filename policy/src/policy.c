/*
 * $Id: $
 *
 * Copyright (c) 2020 Mark T. Uemura <mtu@me.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <polcon.h>     /* /usr/include/polcon.h */

#include <sys/types.h>  /* size_t, ssize_t */
#include <stdarg.h>     /* va_list */
#include <stdint.h>     /* int64_t and ksql(3) */
#include <unistd.h>     /* ssize_t */

#include <stddef.h>     /* NULL */
#include <stdlib.h>     /* EXIT_SUCCESS */
#include <stdio.h>

#include <errno.h>
#include <string.h>
#include <time.h>
#include <uuid.h>

#include <kcgi.h>
#include <kcgihtml.h>

#include <libpq-fe.h>
#include <ctype.h>
#include <err.h>
#include <pwd.h>
#include <limits.h>
#include <openssl/sha.h>

#include <regex.h>
#include <assert.h>     /* for rand_str */

enum page {
	PAGE_INDEX,         /* /index or just / */
	PAGE__MAX
};

void rand_str(char *, size_t);

void rand_str(char *dest, size_t length) {
	char charset[] = "23456789"
					 "abcdefghjkmnpqrstuvwxyz"
					 "ABCDEFGHJKMNPQRSTUVWXYZ";
	while (length-- > 0) {
		size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
		*dest++ = charset[index];
	}
	*dest = '\0';
}

void
cap(char string[]) {
	int l;
	l = 0;
	if (l == 0) {
		if (isalpha(string[l])){
			string[l]= toupper(string[l]);
		}
	}
}

int 
main(void)
{
// HEAD
	/*
	 * CHANGE THIS HERE
	 */
	const char txtDomain[]  = "www.example.com";
	
	const char txtConfirmation[]           = "Thank you for the confirmation.";
	const char txtConfirmationExists[]     = "Thank you but we already received your confirmation on";
	const char txtTokenLengthBad[]         = "There was a problem with the token provided.";
	 
	char str[] = { [13] = '\1' }; // used for import.c and staff.c
	char txt_isp[14], txt_coc[14], txt_dsp[14], txt_gdpr[14], txt_tmp1[14], txt_tmp2[14], txt_tmp3[14], txt_tmp4[14]; // used for import.c and staff.c
	 
	char tmpErrToken[10], tmpErrTokenURL[10], tmpErrHash[10], tmpErrBadToken[10];
	 
	char            txt_session_id   [37]; // (32 alphanumeric characters and four hyphens) + NULL
	unsigned char   txtSHA1          [65]; // txt_session_id_sha1
	unsigned char   txtVerifySHA1    [65]; // copy of txt_session_id_sha1 from DB
	 
	char username[128];
	char firstname[64];
	char lastname[64];
	char string[128];
	char oemail[128];
	char aemail[128];
	char uemail[128];
	char demail[128];
	unsigned char txtTEXTfromURL [14];
	 
	// used for confirmation page
	char txtHASH[65], txtHASH2[65], txtHASHfromURL[65];
	char *x = NULL;
	char *y = NULL;
	char *u = NULL;
	char *v = NULL;
	char *d = NULL;
	char *e = NULL;
	char *f = NULL;
	char token [65];
	char *sub_text;
	 	
	unsigned char txtGUID [37];
	 
	unsigned char txt_session_id_sha256[65];
	unsigned char txt_session_id_sha1  [65];
	 
	struct uuid     guid;
	regex_t         rgx;
	int             nfields, ntuples, i, j, k, status, res;
	char            *guidstr = NULL;
	const char      *hashstr = NULL;
	const char      *pref = NULL;
	pref            = "blowfish,8";
	char            txtCookie[37];
	 
	unsigned char email[128];
	 
	unsigned char db_isp_qry_confirm[2048];
	unsigned char db_isp_set_confirm_timestamp[2048];
	unsigned char db_coc_qry_confirm[2048];
	unsigned char db_coc_set_confirm_timestamp[2048];
	unsigned char db_dsp_qry_confirm[2048];
	unsigned char db_dsp_set_confirm_timestamp[2048];
	unsigned char db_gdpr_qry_confirm[2048];
	unsigned char db_gdpr_set_confirm_timestamp[2048];
	unsigned char db_tmp1_qry_confirm[2048];
	unsigned char db_tmp1_set_confirm_timestamp[2048];
	unsigned char db_tmp2_qry_confirm[2048];
	unsigned char db_tmp2_set_confirm_timestamp[2048];
	unsigned char db_tmp3_qry_confirm[2048];
	unsigned char db_tmp3_set_confirm_timestamp[2048];
	unsigned char db_tmp4_qry_confirm[2048];
	unsigned char db_tmp4_set_confirm_timestamp[2048];
	 
	FILE        *sfp = NULL;
	int         fd = -1;
	 
	char tmpdate[100];
	char txtdate[100];
	time_t current_time = time(NULL);
	struct tm *tm = localtime(&current_time);
	strftime(tmpdate, sizeof(tmpdate), "%F %T", tm);
	 
	char          buf1[_PASSWORD_LEN];
	unsigned char buf2[8096];
	 
	unsigned char ibuf[37];
	unsigned char obuf[32], obuf1[32], obuf2[32], obuf3[32];
	 
	unsigned char txtuasha256  [65];
	unsigned char txtguidsha1  [65];
	unsigned char txtemailsha1 [65];
	unsigned char txtguidsha256[65];
	 
	char tmpError            [10];
	char tmpErrVerifyPassword[10];
	char tmpErrVerifyEmail   [10];
	char tmpErrCompareEmail  [10];
	char tmpErrEmail         [10];
	char tmpErrID            [10];
	char tmpErrPassword      [10];
	char tmpErrLoggedIn      [10];
	char tmpErrText          [10];
	 
	const char *conninfo = NULL;
	PGconn *conn = NULL;
	 
	PGresult *db_isp_res_confirm = NULL;
	PGresult *db_isp_exec_confirm_timestamp = NULL;
	PGresult *db_coc_res_confirm = NULL;
	PGresult *db_coc_exec_confirm_timestamp = NULL;
	PGresult *db_dsp_res_confirm = NULL;
	PGresult *db_dsp_exec_confirm_timestamp = NULL;
	PGresult *db_gdpr_res_confirm = NULL;
	PGresult *db_gdpr_exec_confirm_timestamp = NULL;
	PGresult *db_tmp1_res_confirm = NULL;
	PGresult *db_tmp1_exec_confirm_timestamp = NULL;
	PGresult *db_tmp2_res_confirm = NULL;
	PGresult *db_tmp2_exec_confirm_timestamp = NULL;
	PGresult *db_tmp3_res_confirm = NULL;
	PGresult *db_tmp3_exec_confirm_timestamp = NULL;
	PGresult *db_tmp4_res_confirm = NULL;
	PGresult *db_tmp4_exec_confirm_timestamp = NULL;
// HEAD END
	 
	enum key {
		KEY_TOKEN,
		KEY_COOKIE,
		KEY_COOKIE_INTEGER,
		KEY__MAX
	};
	 
	struct kreq 	r;
	struct kpair   *p = NULL;
	const char     *page = "index";
	 
	static const struct kvalid keys[KEY__MAX] = {
		{kvalid_stringne,	"token"},
		{kvalid_stringne,	"cookie"},
		{kvalid_int,		"integer"},
	};
	 
	if (KCGI_OK != khttp_parse(&r, keys, KEY__MAX, &page, 1, 0))
		return (EXIT_FAILURE);

	if (-1 == pledge("stdio rpath dns inet", NULL)) {
		perror("pledge");
		khttp_free(&r);
		return(EXIT_FAILURE);
	}
 	 	
	/* reset errors */
	strlcpy(tmpError,       "", sizeof(tmpError));
	strlcpy(tmpErrToken,    "", sizeof(tmpErrToken));
	strlcpy(tmpErrTokenURL, "", sizeof(tmpErrTokenURL));
	strlcpy(tmpErrHash,     "", sizeof(tmpErrHash));
	strlcpy(tmpErrBadToken, "", sizeof(tmpErrBadToken));
	 	
	khttp_head(&r, kresps[KRESP_STATUS],          "%s", khttps[KHTTP_200]);
	khttp_head(&r, kresps[KRESP_ALLOW],           "OPTIONS POST");
	khttp_head(&r, kresps[KRESP_CACHE_CONTROL],   "no-cache, no-store, must-revalidate");
	khttp_head(&r, kresps[KRESP_PRAGMA],          "no-cache");
	khttp_head(&r, kresps[KRESP_CONTENT_TYPE],    "%s", kmimetypes[r.mime]);
	khttp_head(&r, kresps[KRESP_X_FRAME_OPTIONS], "DENY");
	 
	khttp_head(&r, "X-Content-Type-Options", "nosniff");
	khttp_head(&r, "X-XSS-Protection", "1; mode=block");
 	 	
	/* the below breaks CSS for some reason so it's commented out for now */
	/* khttp_head(&r, "Content-Security-Policy", "default-src https:"); */
	 	
	if (NULL == r.cookiemap[KEY_COOKIE]) {
		khttp_head(&r, kresps[KRESP_SET_COOKIE], "%s=%s; Path=/; expires=%s", keys[KEY_COOKIE].name, txtGUID, kutil_epoch2str(time(NULL) + TIMEOUT, txtCookie, sizeof(txtCookie)));
	}
	 	
	khttp_body(&r);
	khttp_puts(&r, "<!DOCTYPE html><title>policy confirmation</title>");

	/* body section */
	khttp_puts(&r, "<body style=\"font-family:Verdana;color:#aaaaaa;\">");

	khttp_puts(&r, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">");
	 	
	/* header section */
	khttp_puts(&r, "<div style=\"padding:15px;text-align:center;\">");
	khttp_puts(&r, "<h1>Policy Confirmation Page</h1>");
	khttp_puts(&r, "</div>");
	 	
	/* nav section */
	khttp_puts(&r, "<div style=\"overflow:auto\">");
	 	
	khttp_puts(&r, "<div class=\"main\">");
	khttp_puts(&r, "<table border=\"0\" cellpadding=\"5\">");
	 	
	/* Were we given a token from a form? */
	if ((p = r.fieldmap[KEY_TOKEN])) {
		strlcpy(token, p->parsed.s, sizeof(token));
		if ((int)strlen(token) == 13) {
			/* Do simple length and alphanumeric checks on the token. */
			for (k = 0; k < 13; k++) {
				if (isalnum(token[k]) == 0) {
					strlcpy(tmpErrToken, "bad", sizeof(tmpErrToken));
				}
			}
		} else {
			strlcpy(tmpErrToken, "bad", sizeof(tmpErrToken));
		}
	} else if (r.fieldnmap[KEY_TOKEN])
		strlcpy(tmpErrToken, "empty", sizeof(tmpErrToken));
	else
		strlcpy(tmpErrToken, "empty", sizeof(tmpErrToken));
	 	
	/* Were we given a hash in the URI? */
	if ((r.fullpath)) {
		x = r.fullpath;
		while ((y = strsep(&x, "/")) != NULL) {
			strlcpy(txtHASHfromURL, y, sizeof(txtHASHfromURL));
			if (strcmp(txtHASHfromURL, "") != 0) {
				/* trim off last character '\n' */
				if ((int)strlen(txtHASHfromURL) == 13) {
					/* Do simple length and alphanumeric checks on the token. */
					for (k = 0; k < 13; k++) {
                				if (isalnum(txtHASHfromURL[k]) == 0) {
							strlcpy(tmpErrTokenURL, "bad", sizeof(tmpErrTokenURL));
							khttp_puts(&r, "<tr><td colspan=2>");
							khttp_puts(&r, txtTokenLengthBad);
							khttp_puts(&r, "</td></tr>");
                				}
        				}
				} else {
					strlcpy(tmpErrTokenURL, "bad", sizeof(tmpErrTokenURL));
					khttp_puts(&r, "<tr><td colspan=2>");
					khttp_puts(&r, txtTokenLengthBad);
					khttp_puts(&r, "</td></tr>");
				}
			}
		}
	} else
		strlcpy(tmpErrHash, "empty", sizeof(tmpErrHash));
	 
	/* Did we get a token or a hash? */
	if (strcmp(tmpErrToken, "") == 0 && strcmp(tmpErrTokenURL, "") == 0 && strcmp(token, "") !=0) {
		 
		/*
	 	 * CHANGE THIS HERE
	 	 */
		conninfo = "host=192.168.8.88 dbname=staffdb user=web password=CHANGEME";
		conn = PQconnectdb(conninfo);
		 
		/* set up SQL statements */
		strlcpy(db_isp_qry_confirm, "select txt_isp_confirmed from tbl_staff where txt_isp = ", sizeof(db_isp_qry_confirm));
		strlcat(db_isp_qry_confirm, "'", sizeof(db_isp_qry_confirm));
		strlcat(db_isp_qry_confirm, token, sizeof(db_isp_qry_confirm));
		strlcat(db_isp_qry_confirm, "'", sizeof(db_isp_qry_confirm));
		db_isp_res_confirm = PQexec(conn, db_isp_qry_confirm);
		 
		strlcpy(db_coc_qry_confirm, "select txt_coc_confirmed from tbl_staff where txt_coc = ", sizeof(db_coc_qry_confirm));
		strlcat(db_coc_qry_confirm, "'", sizeof(db_coc_qry_confirm));
		strlcat(db_coc_qry_confirm, token, sizeof(db_coc_qry_confirm));
		strlcat(db_coc_qry_confirm, "'", sizeof(db_coc_qry_confirm));
		db_coc_res_confirm = PQexec(conn, db_coc_qry_confirm);
		 
		strlcpy(db_dsp_qry_confirm, "select txt_dsp_confirmed from tbl_staff where txt_dsp = ", sizeof(db_dsp_qry_confirm));
		strlcat(db_dsp_qry_confirm, "'", sizeof(db_dsp_qry_confirm));
		strlcat(db_dsp_qry_confirm, token, sizeof(db_dsp_qry_confirm));
		strlcat(db_dsp_qry_confirm, "'", sizeof(db_dsp_qry_confirm));
		db_dsp_res_confirm = PQexec(conn, db_dsp_qry_confirm);
		 
		strlcpy(db_gdpr_qry_confirm, "select txt_gdpr_confirmed from tbl_staff where txt_gdpr = ", sizeof(db_gdpr_qry_confirm));
		strlcat(db_gdpr_qry_confirm, "'", sizeof(db_gdpr_qry_confirm));
		strlcat(db_gdpr_qry_confirm, token, sizeof(db_gdpr_qry_confirm));
		strlcat(db_gdpr_qry_confirm, "'", sizeof(db_gdpr_qry_confirm));
		db_gdpr_res_confirm = PQexec(conn, db_gdpr_qry_confirm);
		 
		strlcpy(db_tmp1_qry_confirm, "select txt_tmp1_confirmed from tbl_staff where txt_tmp1 = ", sizeof(db_tmp1_qry_confirm));
		strlcat(db_tmp1_qry_confirm, "'", sizeof(db_tmp1_qry_confirm));
		strlcat(db_tmp1_qry_confirm, token, sizeof(db_tmp1_qry_confirm));
		strlcat(db_tmp1_qry_confirm, "'", sizeof(db_tmp1_qry_confirm));
		db_tmp1_res_confirm = PQexec(conn, db_tmp1_qry_confirm);
		 
		strlcpy(db_tmp2_qry_confirm, "select txt_tmp2_confirmed from tbl_staff where txt_tmp2 = ", sizeof(db_tmp2_qry_confirm));
		strlcat(db_tmp2_qry_confirm, "'", sizeof(db_tmp2_qry_confirm));
		strlcat(db_tmp2_qry_confirm, token, sizeof(db_tmp2_qry_confirm));
		strlcat(db_tmp2_qry_confirm, "'", sizeof(db_tmp2_qry_confirm));
		db_tmp2_res_confirm = PQexec(conn, db_tmp2_qry_confirm);
		 
		strlcpy(db_tmp3_qry_confirm, "select txt_tmp3_confirmed from tbl_staff where txt_tmp3 = ", sizeof(db_tmp3_qry_confirm));
		strlcat(db_tmp3_qry_confirm, "'", sizeof(db_tmp3_qry_confirm));
		strlcat(db_tmp3_qry_confirm, token, sizeof(db_tmp3_qry_confirm));
		strlcat(db_tmp3_qry_confirm, "'", sizeof(db_tmp3_qry_confirm));
		db_tmp3_res_confirm = PQexec(conn, db_tmp3_qry_confirm);
		 
		strlcpy(db_tmp4_qry_confirm, "select txt_tmp4_confirmed from tbl_staff where txt_tmp4 = ", sizeof(db_tmp4_qry_confirm));
		strlcat(db_tmp4_qry_confirm, "'", sizeof(db_tmp4_qry_confirm));
		strlcat(db_tmp4_qry_confirm, token, sizeof(db_tmp4_qry_confirm));
		strlcat(db_tmp4_qry_confirm, "'", sizeof(db_tmp4_qry_confirm));
		db_tmp4_res_confirm = PQexec(conn, db_tmp4_qry_confirm);
		 
		if (PQresultStatus(db_isp_res_confirm) == PGRES_TUPLES_OK) {
			if (PQntuples(db_isp_res_confirm) == 1) {
				nfields = PQnfields(db_isp_res_confirm);
				ntuples = PQntuples(db_isp_res_confirm);
				if (strcmp(PQgetvalue(db_isp_res_confirm, 0, 0), "") == 0) {
						 
					strlcpy(db_isp_set_confirm_timestamp, "UPDATE tbl_staff SET txt_isp_confirmed = ", sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, "'", sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, tmpdate, sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, "'", sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, " WHERE txt_isp = ", sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, "'", sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, token, sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, "'", sizeof(db_isp_set_confirm_timestamp));
					db_isp_exec_confirm_timestamp = PQexec(conn, db_isp_set_confirm_timestamp);
			 		 
					khttp_puts(&r, "<tr><td colspan=2>");
					khttp_puts(&r, txtConfirmation);
					khttp_puts(&r, "</td></tr>");
			 		 
				} else {
					strlcpy(txtdate, PQgetvalue(db_isp_res_confirm, 0, 0), sizeof(txtdate));  
					khttp_puts(&r, "<tr><td colspan=2>");
					khttp_puts(&r, txtConfirmationExists);
					khttp_puts(&r, " ");
					khttp_puts(&r, txtdate);
					khttp_puts(&r, " GMT.");
					khttp_puts(&r, "</td></tr>");
				}
			} else if (PQresultStatus(db_coc_res_confirm) == PGRES_TUPLES_OK) { 
				if (PQntuples(db_coc_res_confirm) == 1) {
					nfields = PQnfields(db_coc_res_confirm);
					ntuples = PQntuples(db_coc_res_confirm);
					if (strcmp(PQgetvalue(db_coc_res_confirm, 0, 0), "") == 0) {
					 
						strlcpy(db_coc_set_confirm_timestamp, "UPDATE tbl_staff SET txt_coc_confirmed = ", sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, "'", sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, tmpdate, sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, "'", sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, " WHERE txt_coc = ", sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, "'", sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, token, sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, "'", sizeof(db_coc_set_confirm_timestamp));
						db_coc_exec_confirm_timestamp = PQexec(conn, db_coc_set_confirm_timestamp);
			 			  
						khttp_puts(&r, "<tr><td colspan=2>");
						khttp_puts(&r, txtConfirmation);
						khttp_puts(&r, "</td></tr>");
			 			  
					} else {
						strlcpy(txtdate, PQgetvalue(db_coc_res_confirm, 0, 0), sizeof(txtdate));  
						khttp_puts(&r, "<tr><td colspan=2>");
						khttp_puts(&r, txtConfirmationExists);
						khttp_puts(&r, " ");
						khttp_puts(&r, txtdate);
						khttp_puts(&r, " GMT.");
						khttp_puts(&r, "</td></tr>");
					}
				} else if (PQresultStatus(db_dsp_res_confirm) == PGRES_TUPLES_OK) { 
					if (PQntuples(db_dsp_res_confirm) == 1) {
						nfields = PQnfields(db_dsp_res_confirm);
						ntuples = PQntuples(db_dsp_res_confirm);
						if (strcmp(PQgetvalue(db_dsp_res_confirm, 0, 0), "") == 0) {
						  
							strlcpy(db_dsp_set_confirm_timestamp, "UPDATE tbl_staff SET txt_dsp_confirmed = ", sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, "'", sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, tmpdate, sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, "'", sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, " WHERE txt_dsp = ", sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, "'", sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, token, sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, "'", sizeof(db_dsp_set_confirm_timestamp));
							db_dsp_exec_confirm_timestamp = PQexec(conn, db_dsp_set_confirm_timestamp);
			 			 	  
							khttp_puts(&r, "<tr><td colspan=2>");
							khttp_puts(&r, txtConfirmation);
							khttp_puts(&r, "</td></tr>");
			 			 	  
						} else {
							strlcpy(txtdate, PQgetvalue(db_dsp_res_confirm, 0, 0), sizeof(txtdate));  
							khttp_puts(&r, "<tr><td colspan=2>");
							khttp_puts(&r, txtConfirmationExists);
							khttp_puts(&r, " ");
							khttp_puts(&r, txtdate);
							khttp_puts(&r, " GMT.");
							khttp_puts(&r, "</td></tr>");
						}
					} else if (PQresultStatus(db_gdpr_res_confirm) == PGRES_TUPLES_OK) { 
						if (PQntuples(db_gdpr_res_confirm) == 1) {
							nfields = PQnfields(db_gdpr_res_confirm);
							ntuples = PQntuples(db_gdpr_res_confirm);
							if (strcmp(PQgetvalue(db_gdpr_res_confirm, 0, 0), "") == 0) {
						 	  
								strlcpy(db_gdpr_set_confirm_timestamp, "UPDATE tbl_staff SET txt_gdpr_confirmed = ", sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, "'", sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, tmpdate, sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, "'", sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, " WHERE txt_gdpr = ", sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, "'", sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, token, sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, "'", sizeof(db_gdpr_set_confirm_timestamp));
								db_gdpr_exec_confirm_timestamp = PQexec(conn, db_gdpr_set_confirm_timestamp);
			 			 	 	  
								khttp_puts(&r, "<tr><td colspan=2>");
								khttp_puts(&r, txtConfirmation);
								khttp_puts(&r, "</td></tr>");
			 			 	 	  
							} else {
								strlcpy(txtdate, PQgetvalue(db_gdpr_res_confirm, 0, 0), sizeof(txtdate));  
								khttp_puts(&r, "<tr><td colspan=2>");
								khttp_puts(&r, txtConfirmationExists);
								khttp_puts(&r, " ");
								khttp_puts(&r, txtdate);
								khttp_puts(&r, " GMT.");
								khttp_puts(&r, "</td></tr>");
							}
						} else if (PQresultStatus(db_tmp1_res_confirm) == PGRES_TUPLES_OK) { 
							if (PQntuples(db_tmp1_res_confirm) == 1) {
								nfields = PQnfields(db_tmp1_res_confirm);
								ntuples = PQntuples(db_tmp1_res_confirm);
								if (strcmp(PQgetvalue(db_tmp1_res_confirm, 0, 0), "") == 0) {
						 	 	  
									strlcpy(db_tmp1_set_confirm_timestamp, "UPDATE tbl_staff SET txt_tmp1_confirmed = ", sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, "'", sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, tmpdate, sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, "'", sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, " WHERE txt_tmp1 = ", sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, "'", sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, token, sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, "'", sizeof(db_tmp1_set_confirm_timestamp));
									db_tmp1_exec_confirm_timestamp = PQexec(conn, db_tmp1_set_confirm_timestamp);
			 			 	 	 	  
									khttp_puts(&r, "<tr><td colspan=2>");
									khttp_puts(&r, txtConfirmation);
									khttp_puts(&r, "</td></tr>");
			 			 	 	 	  
								} else {
									strlcpy(txtdate, PQgetvalue(db_tmp1_res_confirm, 0, 0), sizeof(txtdate));  
									khttp_puts(&r, "<tr><td colspan=2>");
									khttp_puts(&r, txtConfirmationExists);
									khttp_puts(&r, " ");
									khttp_puts(&r, txtdate);
									khttp_puts(&r, " GMT.");
									khttp_puts(&r, "</td></tr>");
								}
							} else if (PQresultStatus(db_tmp2_res_confirm) == PGRES_TUPLES_OK) { 
								if (PQntuples(db_tmp2_res_confirm) == 1) {
									nfields = PQnfields(db_tmp2_res_confirm);
									ntuples = PQntuples(db_tmp2_res_confirm);
									if (strcmp(PQgetvalue(db_tmp2_res_confirm, 0, 0), "") == 0) {
						 	 	 	  
										strlcpy(db_tmp2_set_confirm_timestamp, "UPDATE tbl_staff SET txt_tmp2_confirmed = ", sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, "'", sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, tmpdate, sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, "'", sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, " WHERE txt_tmp2 = ", sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, "'", sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, token, sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, "'", sizeof(db_tmp2_set_confirm_timestamp));
										db_tmp2_exec_confirm_timestamp = PQexec(conn, db_tmp2_set_confirm_timestamp);
			 			 	 	 	 	  
										khttp_puts(&r, "<tr><td colspan=2>");
										khttp_puts(&r, txtConfirmation);
										khttp_puts(&r, "</td></tr>");
			 			 	 	 	 	  
									} else {
										strlcpy(txtdate, PQgetvalue(db_tmp2_res_confirm, 0, 0), sizeof(txtdate));  
										khttp_puts(&r, "<tr><td colspan=2>");
										khttp_puts(&r, txtConfirmationExists);
										khttp_puts(&r, " ");
										khttp_puts(&r, txtdate);
										khttp_puts(&r, " GMT.");
										khttp_puts(&r, "</td></tr>");
									}
								} else if (PQresultStatus(db_tmp3_res_confirm) == PGRES_TUPLES_OK) { 
									if (PQntuples(db_tmp3_res_confirm) == 1) {
										nfields = PQnfields(db_tmp3_res_confirm);
										ntuples = PQntuples(db_tmp3_res_confirm);
										if (strcmp(PQgetvalue(db_tmp3_res_confirm, 0, 0), "") == 0) {
						 	 	 	 	  
											strlcpy(db_tmp3_set_confirm_timestamp, "UPDATE tbl_staff SET txt_tmp3_confirmed = ", sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, "'", sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, tmpdate, sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, "'", sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, " WHERE txt_tmp3 = ", sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, "'", sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, token, sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, "'", sizeof(db_tmp3_set_confirm_timestamp));
											db_tmp3_exec_confirm_timestamp = PQexec(conn, db_tmp3_set_confirm_timestamp);
			 			 	 	 	 	 	  
											khttp_puts(&r, "<tr><td colspan=2>");
											khttp_puts(&r, txtConfirmation);
											khttp_puts(&r, "</td></tr>");
			 			 	 	 	 	 	  
										} else {
											strlcpy(txtdate, PQgetvalue(db_tmp3_res_confirm, 0, 0), sizeof(txtdate));  
											khttp_puts(&r, "<tr><td colspan=2>");
											khttp_puts(&r, txtConfirmationExists);
											khttp_puts(&r, " ");
											khttp_puts(&r, txtdate);
											khttp_puts(&r, " GMT.");
											khttp_puts(&r, "</td></tr>");
										}
									} else if (PQresultStatus(db_tmp4_res_confirm) == PGRES_TUPLES_OK) { 
										if (PQntuples(db_tmp4_res_confirm) == 1) {
											nfields = PQnfields(db_tmp4_res_confirm);
											ntuples = PQntuples(db_tmp4_res_confirm);
											if (strcmp(PQgetvalue(db_tmp4_res_confirm, 0, 0), "") == 0) {
						 	 	 	 	 	  
												strlcpy(db_tmp4_set_confirm_timestamp, "UPDATE tbl_staff SET txt_tmp4_confirmed = ", sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, "'", sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, tmpdate, sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, "'", sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, " WHERE txt_tmp4 = ", sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, "'", sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, token, sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, "'", sizeof(db_tmp4_set_confirm_timestamp));
												db_tmp4_exec_confirm_timestamp = PQexec(conn, db_tmp4_set_confirm_timestamp);
			 			 	 	 	 	 	 	  
												khttp_puts(&r, "<tr><td colspan=2>");
												khttp_puts(&r, txtConfirmation);
												khttp_puts(&r, "</td></tr>");
			 			 	 	 	 	 	 	 
											} else {
												strlcpy(txtdate, PQgetvalue(db_tmp4_res_confirm, 0, 0), sizeof(txtdate));  
												khttp_puts(&r, "<tr><td colspan=2>");
												khttp_puts(&r, txtConfirmationExists);
												khttp_puts(&r, " ");
												khttp_puts(&r, txtdate);
												khttp_puts(&r, " GMT.");
												khttp_puts(&r, "</td></tr>");
											}
										}
									} // tmp4
								} // tmp3
							} // tmp2
						} // tmp1
					} // gdpr
				} // dsp
			} // coc
		} // isp
	} else if (strcmp(tmpErrTokenURL, "") == 0 && strcmp(txtHASHfromURL, "") !=0) {
		/*
	 	 * CHANGE THIS HERE
	 	 */
		conninfo = "host=192.168.8.88 dbname=staffdb user=web password=CHANGEME";
		conn = PQconnectdb(conninfo);
		 
		/* check to see date exists already */
		strlcpy(db_isp_qry_confirm, "select txt_isp_confirmed from tbl_staff where txt_isp = ", sizeof(db_isp_qry_confirm));
		strlcat(db_isp_qry_confirm, "'", sizeof(db_isp_qry_confirm));
		strlcat(db_isp_qry_confirm, txtHASHfromURL, sizeof(db_isp_qry_confirm));
		strlcat(db_isp_qry_confirm, "'", sizeof(db_isp_qry_confirm));
		db_isp_res_confirm = PQexec(conn, db_isp_qry_confirm);
		 
		strlcpy(db_coc_qry_confirm, "select txt_coc_confirmed from tbl_staff where txt_coc = ", sizeof(db_coc_qry_confirm));
		strlcat(db_coc_qry_confirm, "'", sizeof(db_coc_qry_confirm));
		strlcat(db_coc_qry_confirm, txtHASHfromURL, sizeof(db_coc_qry_confirm));
		strlcat(db_coc_qry_confirm, "'", sizeof(db_coc_qry_confirm));
		db_coc_res_confirm = PQexec(conn, db_coc_qry_confirm);
		 
		strlcpy(db_dsp_qry_confirm, "select txt_dsp_confirmed from tbl_staff where txt_dsp = ", sizeof(db_dsp_qry_confirm));
		strlcat(db_dsp_qry_confirm, "'", sizeof(db_dsp_qry_confirm));
		strlcat(db_dsp_qry_confirm, txtHASHfromURL, sizeof(db_dsp_qry_confirm));
		strlcat(db_dsp_qry_confirm, "'", sizeof(db_dsp_qry_confirm));
		db_dsp_res_confirm = PQexec(conn, db_dsp_qry_confirm);
		 
		strlcpy(db_gdpr_qry_confirm, "select txt_gdpr_confirmed from tbl_staff where txt_gdpr = ", sizeof(db_gdpr_qry_confirm));
		strlcat(db_gdpr_qry_confirm, "'", sizeof(db_gdpr_qry_confirm));
		strlcat(db_gdpr_qry_confirm, txtHASHfromURL, sizeof(db_gdpr_qry_confirm));
		strlcat(db_gdpr_qry_confirm, "'", sizeof(db_gdpr_qry_confirm));
		db_gdpr_res_confirm = PQexec(conn, db_gdpr_qry_confirm);
		 
		strlcpy(db_tmp1_qry_confirm, "select txt_tmp1_confirmed from tbl_staff where txt_tmp1 = ", sizeof(db_tmp1_qry_confirm));
		strlcat(db_tmp1_qry_confirm, "'", sizeof(db_tmp1_qry_confirm));
		strlcat(db_tmp1_qry_confirm, txtHASHfromURL, sizeof(db_tmp1_qry_confirm));
		strlcat(db_tmp1_qry_confirm, "'", sizeof(db_tmp1_qry_confirm));
		db_tmp1_res_confirm = PQexec(conn, db_tmp1_qry_confirm);
		 
		strlcpy(db_tmp2_qry_confirm, "select txt_tmp2_confirmed from tbl_staff where txt_tmp2 = ", sizeof(db_tmp2_qry_confirm));
		strlcat(db_tmp2_qry_confirm, "'", sizeof(db_tmp2_qry_confirm));
		strlcat(db_tmp2_qry_confirm, txtHASHfromURL, sizeof(db_tmp2_qry_confirm));
		strlcat(db_tmp2_qry_confirm, "'", sizeof(db_tmp2_qry_confirm));
		db_tmp2_res_confirm = PQexec(conn, db_tmp2_qry_confirm);
		 
		strlcpy(db_tmp3_qry_confirm, "select txt_tmp3_confirmed from tbl_staff where txt_tmp3 = ", sizeof(db_tmp3_qry_confirm));
		strlcat(db_tmp3_qry_confirm, "'", sizeof(db_tmp3_qry_confirm));
		strlcat(db_tmp3_qry_confirm, txtHASHfromURL, sizeof(db_tmp3_qry_confirm));
		strlcat(db_tmp3_qry_confirm, "'", sizeof(db_tmp3_qry_confirm));
		db_tmp3_res_confirm = PQexec(conn, db_tmp3_qry_confirm);
		 
		strlcpy(db_tmp4_qry_confirm, "select txt_tmp4_confirmed from tbl_staff where txt_tmp4 = ", sizeof(db_tmp4_qry_confirm));
		strlcat(db_tmp4_qry_confirm, "'", sizeof(db_tmp4_qry_confirm));
		strlcat(db_tmp4_qry_confirm, txtHASHfromURL, sizeof(db_tmp4_qry_confirm));
		strlcat(db_tmp4_qry_confirm, "'", sizeof(db_tmp4_qry_confirm));
		db_tmp4_res_confirm = PQexec(conn, db_tmp4_qry_confirm);
		 
		if (PQresultStatus(db_isp_res_confirm) == PGRES_TUPLES_OK) {
			if (PQntuples(db_isp_res_confirm) == 1) {
				nfields = PQnfields(db_isp_res_confirm);
				ntuples = PQntuples(db_isp_res_confirm);
				if (strcmp(PQgetvalue(db_isp_res_confirm, 0, 0), "") == 0) {
					 
					strlcpy(db_isp_set_confirm_timestamp, "UPDATE tbl_staff SET txt_isp_confirmed = ", sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, "'", sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, tmpdate, sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, "'", sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, " WHERE txt_isp = ", sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, "'", sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, txtHASHfromURL, sizeof(db_isp_set_confirm_timestamp));
					strlcat(db_isp_set_confirm_timestamp, "'", sizeof(db_isp_set_confirm_timestamp));
					db_isp_exec_confirm_timestamp = PQexec(conn, db_isp_set_confirm_timestamp);
					 
					khttp_puts(&r, "<tr><td colspan=2>");
					khttp_puts(&r, txtConfirmation);
					khttp_puts(&r, "</td></tr>");
					 
				} else {
					strlcpy(txtdate, PQgetvalue(db_isp_res_confirm, 0, 0), sizeof(txtdate));  
					khttp_puts(&r, "<tr><td colspan=2>");
					khttp_puts(&r, txtConfirmationExists);
					khttp_puts(&r, " ");
					khttp_puts(&r, txtdate);
					khttp_puts(&r, " GMT.");
					khttp_puts(&r, "</td></tr>");
				}
			} else if (PQresultStatus(db_coc_res_confirm) == PGRES_TUPLES_OK) { 
				if (PQntuples(db_coc_res_confirm) == 1) {
					nfields = PQnfields(db_coc_res_confirm);
					ntuples = PQntuples(db_coc_res_confirm);
					if (strcmp(PQgetvalue(db_coc_res_confirm, 0, 0), "") == 0) {
						  
						strlcpy(db_coc_set_confirm_timestamp, "UPDATE tbl_staff SET txt_coc_confirmed = ", sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, "'", sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, tmpdate, sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, "'", sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, " WHERE txt_coc = ", sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, "'", sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, txtHASHfromURL, sizeof(db_coc_set_confirm_timestamp));
						strlcat(db_coc_set_confirm_timestamp, "'", sizeof(db_coc_set_confirm_timestamp));
						db_coc_exec_confirm_timestamp = PQexec(conn, db_coc_set_confirm_timestamp);
						 
						khttp_puts(&r, "<tr><td colspan=2>");
						khttp_puts(&r, txtConfirmation);
						khttp_puts(&r, "</td></tr>");
						 
					} else {
						strlcpy(txtdate, PQgetvalue(db_coc_res_confirm, 0, 0), sizeof(txtdate));  
						khttp_puts(&r, "<tr><td colspan=2>");
						khttp_puts(&r, txtConfirmationExists);
						khttp_puts(&r, " ");
						khttp_puts(&r, txtdate);
						khttp_puts(&r, " GMT.");
						khttp_puts(&r, "</td></tr>");
					}
				} else if (PQresultStatus(db_dsp_res_confirm) == PGRES_TUPLES_OK) { 
					if (PQntuples(db_dsp_res_confirm) == 1) {
						nfields = PQnfields(db_dsp_res_confirm);
						ntuples = PQntuples(db_dsp_res_confirm);
						if (strcmp(PQgetvalue(db_dsp_res_confirm, 0, 0), "") == 0) {
						 	  
							strlcpy(db_dsp_set_confirm_timestamp, "UPDATE tbl_staff SET txt_dsp_confirmed = ", sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, "'", sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, tmpdate, sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, "'", sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, " WHERE txt_dsp = ", sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, "'", sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, txtHASHfromURL, sizeof(db_dsp_set_confirm_timestamp));
							strlcat(db_dsp_set_confirm_timestamp, "'", sizeof(db_dsp_set_confirm_timestamp));
							db_dsp_exec_confirm_timestamp = PQexec(conn, db_dsp_set_confirm_timestamp);
							  
							khttp_puts(&r, "<tr><td colspan=2>");
							khttp_puts(&r, txtConfirmation);
							khttp_puts(&r, "</td></tr>");
							 
						} else {
							strlcpy(txtdate, PQgetvalue(db_dsp_res_confirm, 0, 0), sizeof(txtdate));  
							khttp_puts(&r, "<tr><td colspan=2>");
							khttp_puts(&r, txtConfirmationExists);
							khttp_puts(&r, " ");
							khttp_puts(&r, txtdate);
							khttp_puts(&r, " GMT.");
							khttp_puts(&r, "</td></tr>");
						}
					} else if (PQresultStatus(db_gdpr_res_confirm) == PGRES_TUPLES_OK) { 
						if (PQntuples(db_gdpr_res_confirm) == 1) {
							nfields = PQnfields(db_gdpr_res_confirm);
							ntuples = PQntuples(db_gdpr_res_confirm);
							if (strcmp(PQgetvalue(db_gdpr_res_confirm, 0, 0), "") == 0) {
						 	  
								strlcpy(db_gdpr_set_confirm_timestamp, "UPDATE tbl_staff SET txt_gdpr_confirmed = ", sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, "'", sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, tmpdate, sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, "'", sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, " WHERE txt_gdpr = ", sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, "'", sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, txtHASHfromURL, sizeof(db_gdpr_set_confirm_timestamp));
								strlcat(db_gdpr_set_confirm_timestamp, "'", sizeof(db_gdpr_set_confirm_timestamp));
								db_gdpr_exec_confirm_timestamp = PQexec(conn, db_gdpr_set_confirm_timestamp);
							 	 
								khttp_puts(&r, "<tr><td colspan=2>");
								khttp_puts(&r, txtConfirmation);
								khttp_puts(&r, "</td></tr>");
								  
							} else {
								strlcpy(txtdate, PQgetvalue(db_gdpr_res_confirm, 0, 0), sizeof(txtdate));  
								khttp_puts(&r, "<tr><td colspan=2>");
								khttp_puts(&r, txtConfirmationExists);
								khttp_puts(&r, " ");
								khttp_puts(&r, txtdate);
								khttp_puts(&r, " GMT.");
								khttp_puts(&r, "</td></tr>");
							}
						} else if (PQresultStatus(db_tmp1_res_confirm) == PGRES_TUPLES_OK) { 
							if (PQntuples(db_tmp1_res_confirm) == 1) {
								nfields = PQnfields(db_tmp1_res_confirm);
								ntuples = PQntuples(db_tmp1_res_confirm);
								if (strcmp(PQgetvalue(db_tmp1_res_confirm, 0, 0), "") == 0) {
						 	 	  
									strlcpy(db_tmp1_set_confirm_timestamp, "UPDATE tbl_staff SET txt_tmp1_confirmed = ", sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, "'", sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, tmpdate, sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, "'", sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, " WHERE txt_tmp1 = ", sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, "'", sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, txtHASHfromURL, sizeof(db_tmp1_set_confirm_timestamp));
									strlcat(db_tmp1_set_confirm_timestamp, "'", sizeof(db_tmp1_set_confirm_timestamp));
									db_tmp1_exec_confirm_timestamp = PQexec(conn, db_tmp1_set_confirm_timestamp);
							 		  
									khttp_puts(&r, "<tr><td colspan=2>");
									khttp_puts(&r, txtConfirmation);
									khttp_puts(&r, "</td></tr>");
								 	  
								} else {
									strlcpy(txtdate, PQgetvalue(db_tmp1_res_confirm, 0, 0), sizeof(txtdate));  
									khttp_puts(&r, "<tr><td colspan=2>");
									khttp_puts(&r, txtConfirmationExists);
									khttp_puts(&r, " ");
									khttp_puts(&r, txtdate);
									khttp_puts(&r, " GMT.");
									khttp_puts(&r, "</td></tr>");
								}
							} else if (PQresultStatus(db_tmp2_res_confirm) == PGRES_TUPLES_OK) { 
								if (PQntuples(db_tmp2_res_confirm) == 1) {
									nfields = PQnfields(db_tmp2_res_confirm);
									ntuples = PQntuples(db_tmp2_res_confirm);
									if (strcmp(PQgetvalue(db_tmp2_res_confirm, 0, 0), "") == 0) {
						 	 	 	  
										strlcpy(db_tmp2_set_confirm_timestamp, "UPDATE tbl_staff SET txt_tmp2_confirmed = ", sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, "'", sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, tmpdate, sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, "'", sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, " WHERE txt_tmp2 = ", sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, "'", sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, txtHASHfromURL, sizeof(db_tmp2_set_confirm_timestamp));
										strlcat(db_tmp2_set_confirm_timestamp, "'", sizeof(db_tmp2_set_confirm_timestamp));
										db_tmp2_exec_confirm_timestamp = PQexec(conn, db_tmp2_set_confirm_timestamp);
							 		 	  
										khttp_puts(&r, "<tr><td colspan=2>");
										khttp_puts(&r, txtConfirmation);
										khttp_puts(&r, "</td></tr>");
								 	 	 
									} else {
										strlcpy(txtdate, PQgetvalue(db_tmp2_res_confirm, 0, 0), sizeof(txtdate));  
										khttp_puts(&r, "<tr><td colspan=2>");
										khttp_puts(&r, txtConfirmationExists);
										khttp_puts(&r, " ");
										khttp_puts(&r, txtdate);
										khttp_puts(&r, " GMT.");
										khttp_puts(&r, "</td></tr>");
									}
								} else if (PQresultStatus(db_tmp3_res_confirm) == PGRES_TUPLES_OK) { 
									if (PQntuples(db_tmp3_res_confirm) == 1) {
										nfields = PQnfields(db_tmp3_res_confirm);
										ntuples = PQntuples(db_tmp3_res_confirm);
										if (strcmp(PQgetvalue(db_tmp3_res_confirm, 0, 0), "") == 0) {
						 	 	 	 	  
											strlcpy(db_tmp3_set_confirm_timestamp, "UPDATE tbl_staff SET txt_tmp3_confirmed = ", sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, "'", sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, tmpdate, sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, "'", sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, " WHERE txt_tmp3 = ", sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, "'", sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, txtHASHfromURL, sizeof(db_tmp3_set_confirm_timestamp));
											strlcat(db_tmp3_set_confirm_timestamp, "'", sizeof(db_tmp3_set_confirm_timestamp));
											db_tmp3_exec_confirm_timestamp = PQexec(conn, db_tmp3_set_confirm_timestamp);
							 		 	 	  
											khttp_puts(&r, "<tr><td colspan=2>");
											khttp_puts(&r, txtConfirmation);
											khttp_puts(&r, "</td></tr>");
								 	 		  
										} else {
											strlcpy(txtdate, PQgetvalue(db_tmp3_res_confirm, 0, 0), sizeof(txtdate));  
											khttp_puts(&r, "<tr><td colspan=2>");
											khttp_puts(&r, txtConfirmationExists);
											khttp_puts(&r, " ");
											khttp_puts(&r, txtdate);
											khttp_puts(&r, " GMT.");
											khttp_puts(&r, "</td></tr>");
										}
									} else if (PQresultStatus(db_tmp4_res_confirm) == PGRES_TUPLES_OK) { 
										if (PQntuples(db_tmp4_res_confirm) == 1) {
											nfields = PQnfields(db_tmp4_res_confirm);
											ntuples = PQntuples(db_tmp4_res_confirm);
											if (strcmp(PQgetvalue(db_tmp4_res_confirm, 0, 0), "") == 0) {
						 	 	 	 	 	  
												strlcpy(db_tmp4_set_confirm_timestamp, "UPDATE tbl_staff SET txt_tmp4_confirmed = ", sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, "'", sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, tmpdate, sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, "'", sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, " WHERE txt_tmp4 = ", sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, "'", sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, txtHASHfromURL, sizeof(db_tmp4_set_confirm_timestamp));
												strlcat(db_tmp4_set_confirm_timestamp, "'", sizeof(db_tmp4_set_confirm_timestamp));
												db_tmp4_exec_confirm_timestamp = PQexec(conn, db_tmp4_set_confirm_timestamp);
							 		 	 	 	  
												khttp_puts(&r, "<tr><td colspan=2>");
												khttp_puts(&r, txtConfirmation);
												khttp_puts(&r, "</td></tr>");
								 	 		 	  
											} else {
												strlcpy(txtdate, PQgetvalue(db_tmp4_res_confirm, 0, 0), sizeof(txtdate));  
												khttp_puts(&r, "<tr><td colspan=2>");
												khttp_puts(&r, txtConfirmationExists);
												khttp_puts(&r, " ");
												khttp_puts(&r, txtdate);
												khttp_puts(&r, " GMT.");
												khttp_puts(&r, "</td></tr>");
											}
										}
									} // tmp4
								} // tmp3
							} // tmp2
						} // tmp1
					} // gdpr
				} // dsp
			} // coc
		} // isp
	} // else if (strcmp(tmpErrTokenURL, "") == 0 && strcmp(txtHASHfromURL, "") !=0) {
	 
	if (strcmp(tmpErrToken, "bad") == 0) {
		khttp_puts(&r, "<tr><td colspan=2>");
		khttp_puts(&r, txtTokenLengthBad);
		khttp_puts(&r, "</td></tr>");
	}
	 
	if (strcmp(txtTokenLengthBad, "bad") == 0) {
		khttp_puts(&r, "<tr><td colspan=2>");
		khttp_puts(&r, txtTokenLengthBad);
		khttp_puts(&r, "</td></tr>");
	}
	 
	khttp_puts(&r, "</table>");
	khttp_puts(&r, "</div>");
	 	
	khttp_puts(&r, "</div>");
	khttp_puts(&r, "</body>");
	khttp_puts(&r, "</html>");
	 	
// TAIL
	PQclear(db_isp_res_confirm);
	PQclear(db_isp_exec_confirm_timestamp);
	PQclear(db_coc_res_confirm);
	PQclear(db_coc_exec_confirm_timestamp);
	PQclear(db_dsp_res_confirm);
	PQclear(db_dsp_exec_confirm_timestamp);
	PQclear(db_gdpr_res_confirm);
	PQclear(db_gdpr_exec_confirm_timestamp);
	PQclear(db_tmp1_res_confirm);
	PQclear(db_tmp1_exec_confirm_timestamp);
	PQclear(db_tmp2_res_confirm);
	PQclear(db_tmp2_exec_confirm_timestamp);
	PQclear(db_tmp3_res_confirm);
	PQclear(db_tmp3_exec_confirm_timestamp);
	PQclear(db_tmp4_res_confirm);
	PQclear(db_tmp4_exec_confirm_timestamp);
// TAIL END
	 
	PQfinish(conn); /* Close connection */
	khttp_free(&r);
	 
	return (EXIT_SUCCESS);
}	/* end main */
