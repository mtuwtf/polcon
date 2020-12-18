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

#include <polcon.h>             /* /usr/include/polcon.h */

#include <sys/types.h>          /* size_t, ssize_t */
#include <stdarg.h>             /* va_list */
#include <stdint.h>             /* int64_t and ksql(3) */
#include <unistd.h>             /* ssize_t and pledge */

#include <stddef.h>             /* NULL */
#include <stdlib.h>             /* EXIT_SUCCESS */
#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <time.h>
#include <uuid.h>

#include <kcgi.h>
#include <kcgihtml.h>

#include <libpq-fe.h>
#include <ctype.h>               /* toupper */
#include <err.h>
#include <pwd.h>
#include <limits.h>
#include <openssl/sha.h>

#include <regex.h>
#include <assert.h>               /* for rand_str */

#define BUFFER_SIZE (16 * 1024)   /* use 16K buffers - import */

enum page {
	PAGE_INDEX,                   /* /index or just / */
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

char *itobase10(char *buf, int value) {
	int ret = snprintf(buf, sizeof(buf), "%d", value);
	if (ret == -1 || ret >= sizeof(buf))
		errx(1, "snprintf error");
	 
	return buf;
}

int 
main(int argc, char *argv[])
{
//HEAD
	/*
	 * CHANGE THIS HERE
	 */
	const char txtDomain[]  = "www.example.com";
	
	const char txtConfirmation[]           = "Thank you for the confirmation.";
	const char txtConfirmationExists[]     = "Thank you but we already received your confirmation on";
	const char txtTokenLengthBad[]         = "There was a problem with the token provided.";
	 
	char str[] = { [13] = '\1' };
	char txt_isp[14], txt_coc[14], txt_dsp[14], txt_gdpr[14], txt_tmp1[14], txt_tmp2[14], txt_tmp3[14], txt_tmp4[14]; // used for import.c and staff.c
	 
	const char errConnectionProblem[]      = "There is a problem with the connection.";
	 
	char            txt_session_id   [37]; // (32 alphanumeric characters and four hyphens) + NULL
	unsigned char   txtSHA1          [65]; // txt_session_id_sha1
	unsigned char   txtVerifySHA1    [65]; // copy of txt_session_id_sha1 from DB
	unsigned char   txtSHA256        [65]; // txt_session_id_sha256
	unsigned char   txtVerifySHA256  [65]; // copy of txt_session_id_sha256 from DB
	 
	char string[128];
	char oemail[128];
	char aemail[128];
	char uemail[128];
	char demail[128];
	unsigned char txtTEXTfromURL [14];
	 
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
	 
	unsigned char txt_session_id_sha256[65]; // these will hold the hashes of the txt_session_id
	unsigned char txt_session_id_sha1  [65];
	 
	unsigned char   tmp1[65], tmp2[65], tmp3[65];
	 
	struct uuid     guid;
	regex_t         rgx;
	int             nfields, ntuples, i, j, k, status, res, ret, max_qa_len = 127;
	int             tmp_new_email, tmp_dup_email;
	char            txt_new_email[4], txt_dup_email[4];
	char            *guidstr = NULL;
	const char      *hashstr = NULL;
	char            buffer[_PASSWORD_LEN];
	const char      *pref = NULL;
	pref            = "blowfish,8";
	char            txtCookie[37];
	 
	unsigned char email[128];
	 
	char db_create_staff[2048];
	char db_search_staff[2048];
	char db_set_confirm[2048];
	char db_qry_confirm[2048];
	char db_set_confirm_timestamp[2048];
	char db_qry_questions[2048];
	 
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
	 
	char db_qry_staff_from_email[2048];
	char db_qry_staff_from_email_sha1[2048];
	 
	char strEmail[128];
	char strEmailSHA1[128];
	 
	FILE        *sfp = NULL;
	int         fd = -1;
	 
	char tmpdate[100]; // store date-time
	char txtdate[100]; // used to retrieve date from DB
	time_t 		current_time = time(NULL);
	struct tm *tm = localtime(&current_time);
	strftime(tmpdate, sizeof(tmpdate), "%F %T", tm);
	 
	char sesdate[100]; // store date-time of session expiry
	time_t session_time = time(NULL) + TIMEOUT;
	struct tm *ses = localtime(&session_time);
	strftime(sesdate, sizeof(sesdate), "%F %T", ses);
	 
	char buf1[_PASSWORD_LEN];
	unsigned char buf2[8096];
	 
	// For the hash function. The txt_session_id goes into this variable.
	unsigned char ibuf[37];
	unsigned char obuf[32], obuf1[32], obuf2[32], obuf3[32];
	 
	unsigned char txtguidsha1[65];
	unsigned char txtemailsha1[65];
	 
	char tmpError[10];
	 
	const char *conninfo = NULL;
	PGconn *conn = NULL;

	PGresult *db_res_staff_from_email = NULL;
	PGresult *db_res_staff_from_email_sha1 = NULL;
	 
	PGresult *db_exec_create = NULL;
	PGresult *db_exec_confirm_timestamp = NULL;
	PGresult *db_exec_search = NULL;
//HEAD END
	 
	enum key {
		KEY_EMAIL,
		KEY_COOKIE,
		KEY_COOKIE_INTEGER,
		KEY__MAX
	};
	 
	struct 			kreq	r;
	struct 			kpair   *p = NULL;
	const char     *page = "index";
	 
	static const struct kvalid keys[KEY__MAX] = {
		{kvalid_email,    "email"},
		{kvalid_stringne, "cookie"},	/* KEY_COOKIE */
		{kvalid_int,      "integer"}, 	/* KEY_COOKIE_INTEGER */
	};
	 
	if (KCGI_OK != khttp_parse(&r, keys, KEY__MAX, &page, 1, 0))
		return (EXIT_FAILURE);
	 
	if (-1 == pledge("stdio rpath wpath cpath dns inet unveil", NULL)) {
		perror("pledge");
		khttp_free(&r);
		return(EXIT_FAILURE);
	}
	 
	/*
	 * Make sure this file exists in
	 * /var/www/cgi-bin/staff.txt
	 * with emails; one per line.
	 * chown root:www /var/www/cgi-bin/staff.txt
	 * chmod 444 /var/www/cgi-bin/staff.txt
	 */
	if (unveil("./staff.txt", "r") == -1)
		warn("unveil");
	 
	sfp = fopen("./staff.txt", "r");
	errno = 0;
	 
	/*
	 * You can add some 5500 emails with emails[5500][5500]
	 * but beyond this you will need to increase system limits.
	 * Tested on OpenBSD 6.7 amd64.
	 */
	char    emails[500][500]; // LIMIT - be careful here. over 500 records will crash.
	size_t  n = 0;
	 
	strlcpy(tmpError, "", sizeof(tmpError)); // reset errors
	 
	khttp_head(&r, kresps[KRESP_STATUS],          "%s", khttps[KHTTP_200]);
	khttp_head(&r, kresps[KRESP_CACHE_CONTROL],   "no-cache, no-store, must-revalidate");
	khttp_head(&r, kresps[KRESP_PRAGMA],          "no-cache");
	khttp_head(&r, kresps[KRESP_CONTENT_TYPE],    "%s", kmimetypes[r.mime]);
	khttp_head(&r, kresps[KRESP_X_FRAME_OPTIONS], "DENY");
	 
	khttp_head(&r, "X-Content-Type-Options", "nosniff");
	khttp_head(&r, "X-XSS-Protection", "1; mode=block");
	 
	uuid_create(&guid, &status); // used to create a guid
	uuid_to_string(&guid, &guidstr, &status);
	 
	if (status == uuid_s_ok) {
		strlcpy(txtGUID, guidstr, sizeof(txtGUID));
		SHA1(txtGUID, strlen(txtGUID), obuf1);
		for (j = 0; j < 20; j++) { // generate the SHA1 hash
			snprintf((char *) &tmp1, sizeof((char *) & tmp1), "%02x", obuf1[j]);
			strlcat(txtguidsha1, tmp1, sizeof(txtguidsha1));
		}
		strlcpy(txtSHA1, txtguidsha1, sizeof(txtSHA1));
	}
	 
	if (NULL == r.cookiemap[KEY_COOKIE]) {
		khttp_head(&r, kresps[KRESP_SET_COOKIE], "%s=%s; Path=/; expires=%s", keys[KEY_COOKIE].name, txtGUID, khttp_epoch2str(time(NULL) + 60 + 60, txtCookie, sizeof(txtCookie)));
	}
	 
	khttp_body(&r);
	 
	khttp_puts(&r, "<!DOCTYPE html><title>Import Page</title>");
	khttp_puts(&r, "<body style=\"font-family:Verdana;color:#aaaaaa;\">");
	 
	khttp_puts(&r, "<style> * { box-sizing: border-box; } ");
	khttp_puts(&r, ".menu { float:left; width:20%; text-align:center; } ");
	khttp_puts(&r, ".menu a { padding:8px; margin-top:7px; display:block; width:100%; color:black; } ");
	khttp_puts(&r, ".main { float:left; width:60%; padding:0 20px; } ");
	khttp_puts(&r, ".right { float:left; width:20%; padding:15px; margin-top:7px; text-align:center; } ");
	khttp_puts(&r, "@media only screen and (max-width:620px) { .menu, .main, .right { width:100%; } } ");
	khttp_puts(&r, "</style> ");
	 
	khttp_puts(&r, "<div style=\"background-color:#e5e5e5;padding:10px;text-align:center;\">");
	khttp_puts(&r, "<div style=\"padding:15px;text-align:center;\">");
	khttp_puts(&r, "<h3>Import Page</h3>");
	khttp_puts(&r, "</div>");
	 
	khttp_puts(&r, "<div style=\"overflow:auto\">");
	khttp_puts(&r, "<div class=\"menu\">");
	khttp_puts(&r, "<a href=https://");
	khttp_puts(&r, txtDomain);
	khttp_puts(&r, "/import.html>Back</a></br>");
	khttp_puts(&r, "</div>");
	 
	khttp_puts(&r, "<div class=\"main\">");
	 
	if (strcmp(tmpError, "") == 0) {
		/*
		 * CHANGE THIS HERE
		 */
		conninfo = "host=192.168.8.88 dbname=staffdb user=web password=CHANGEME";
		conn = PQconnectdb(conninfo); // Make a connection to the database
		 
		if (PQstatus(conn) != CONNECTION_OK) {
			khttp_puts(&r, "<p>");
			khttp_puts(&r, errConnectionProblem);
			khttp_puts(&r, "</p>");
		} else {
			if (sfp == NULL) {
				perror("Error: import - unable to open file staff.txt.");
				khttp_puts(&r, "<p>");
				khttp_puts(&r, "import error");
				khttp_puts(&r, "</p>");
			} else {
				tmp_new_email = 0;
				tmp_dup_email = 0;
				strlcpy(txtSHA1, "", sizeof(txtSHA1));
				/*
				 * This may need to be increased depending
				 * on the longest email in your list.
				 */
				while (fscanf(sfp, "%50s", emails[n]) == 1) {
					strlcpy(db_qry_staff_from_email, "", sizeof(db_qry_staff_from_email));
					strlcpy(oemail, emails[n], sizeof(oemail));
					 
					strlcpy(tmp3,  "", sizeof(tmp3));
					strlcpy(obuf3, "", sizeof(obuf3));
					strlcpy(txtemailsha1, "", sizeof(txtemailsha1));
					strlcpy(strEmailSHA1, "", sizeof(strEmailSHA1));
					SHA1(oemail, strlen(oemail), obuf3);
					for (i = 0; i < 20; i++) {
						snprintf((char *) &tmp3, sizeof((char *) & tmp3), "%02x", obuf3[i]);
						strlcat(txtemailsha1, tmp3, sizeof(txtemailsha1));
					}
					strlcpy(strEmailSHA1, txtemailsha1, sizeof(strEmailSHA1));
					 
					// check for existence of account by sha1 hash of email
					strlcpy(db_qry_staff_from_email_sha1, "SELECT FROM tbl_staff WHERE txt_staff_email_sha1 = ", sizeof(db_qry_staff_from_email_sha1));
					strlcat(db_qry_staff_from_email_sha1, "'", sizeof(db_qry_staff_from_email_sha1));
					strlcat(db_qry_staff_from_email_sha1, strEmailSHA1, sizeof(db_qry_staff_from_email_sha1));
					strlcat(db_qry_staff_from_email_sha1, "'", sizeof(db_qry_staff_from_email_sha1));
					db_res_staff_from_email_sha1 = PQexec(conn, db_qry_staff_from_email_sha1);
					 
					if (PQresultStatus(db_res_staff_from_email_sha1) != PGRES_TUPLES_OK) {
						khttp_puts(&r, "<tr><td colspan=2>");
						khttp_puts(&r, "Houston");
						khttp_puts(&r, "</td></tr>");
					} else if (PQntuples(db_res_staff_from_email_sha1) == 0) {
						// If you get here, then start to insert a new record.
						strlcpy(db_create_staff, "INSERT INTO tbl_staff (txt_staff_guid, txt_staff_guid_sha1, txt_staff_email, txt_staff_email_sha1, txt_created, txt_isp, txt_coc, txt_dsp, txt_gdpr, txt_tmp1, txt_tmp2, txt_tmp3, txt_tmp4) VALUES (", sizeof(db_create_staff));
						uuid_create(&guid, &status); // used to create a guid
						uuid_to_string(&guid, &guidstr, &status);
						  
						strlcpy(txtGUID, guidstr, sizeof(txtGUID));
						SHA1(txtGUID, strlen(txtGUID), obuf2);
						strlcpy(txtSHA1, "", sizeof(txtSHA1));
						strlcpy(txtguidsha1, "", sizeof(txtguidsha1));
						strlcpy(tmp2,  "", sizeof(tmp2));
						strlcpy(obuf2, "", sizeof(obuf2));
						for (j = 0; j < 20; j++) {
							snprintf((char *) &tmp2, sizeof((char *) & tmp2), "%02x", obuf2[j]);
							strlcat(txtguidsha1, tmp2, sizeof(txtguidsha1));
						}
						strlcpy(txtSHA1, txtguidsha1, sizeof(txtSHA1));
						strlcpy(txtguidsha1, "", sizeof(txtguidsha1));
						  
						// db_create_staff + txtGUID + oemail + 0 + datestamp
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						strlcat(db_create_staff, txtGUID, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						strlcat(db_create_staff, txtSHA1, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						  
						strlcpy(oemail, emails[n], sizeof(oemail));
						 
						if ((sub_text = strchr(emails[n], '@')) != NULL)
							strlcpy(demail, sub_text, sizeof(demail));
						 
						e = emails[n];
						/* anonymized the email. */
						if ((f = strsep(&e, "@")) != NULL) {
							strlcpy(aemail, "", sizeof(aemail));
							strlcpy(uemail, f, sizeof(uemail));
							strlcpy(aemail, &uemail[0], sizeof(aemail) - (sizeof(aemail)-2));
							strlcat(aemail, "...", sizeof(aemail));
							strlcat(aemail, &uemail[(int)strlen(uemail)-1], sizeof(aemail));
							strlcat(aemail, demail, sizeof(aemail));
						}
						 
						khttp_puts(&r, "<p>");
						khttp_puts(&r, aemail);
						khttp_puts(&r, "</p>");
						 
						strlcat(db_create_staff, aemail, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						 
						strlcpy(tmp3,  "", sizeof(tmp3));
						strlcpy(obuf3, "", sizeof(obuf3));
						strlcpy(txtemailsha1, "", sizeof(txtemailsha1));
						strlcpy(strEmailSHA1, "", sizeof(strEmailSHA1));
						SHA1(oemail, strlen(oemail), obuf3);
						for (i = 0; i < 20; i++) {
							snprintf((char *) &tmp3, sizeof((char *) & tmp3), "%02x", obuf3[i]);
							strlcat(txtemailsha1, tmp3, sizeof(txtemailsha1));
						}
						strlcpy(strEmailSHA1, txtemailsha1, sizeof(strEmailSHA1));
						 
						/* db_create_staff + strEmailSHA1 */
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						strlcat(db_create_staff, strEmailSHA1, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						 
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						strlcat(db_create_staff, tmpdate, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						  
						/*
						 * Get some random text for txt_isp, txt_coc, txt_dsp, txt_gdpr,
						 * txt_tmp1, txt_tmp2, txt_tmp3 and txt_tmp4 respectively.
						 */
						  
						rand_str(str, sizeof str - 1);  /* for txt_isp */
						assert(str[13] == '\0');
						strlcpy(txt_isp, str, sizeof(txt_isp));
						strlcat(db_create_staff, txt_isp, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						  
						rand_str(str, sizeof str - 1);  /* for txt_coc */
						assert(str[13] == '\0');
						strlcpy(txt_coc, str, sizeof(txt_coc));
						strlcat(db_create_staff, txt_coc, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						  
						rand_str(str, sizeof str - 1);  /* for txt_dsp */
						assert(str[13] == '\0');
						strlcpy(txt_dsp, str, sizeof(txt_dsp));
						strlcat(db_create_staff, txt_dsp, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						  
						rand_str(str, sizeof str - 1);  /* for txt_gdpr */
						assert(str[13] == '\0');
						strlcpy(txt_gdpr, str, sizeof(txt_gdpr));
						strlcat(db_create_staff, txt_gdpr, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						  
						rand_str(str, sizeof str - 1);  /* for txt_tmp1 */
						assert(str[13] == '\0');
						strlcpy(txt_tmp1, str, sizeof(txt_tmp1));
						strlcat(db_create_staff, txt_tmp1, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						  
						rand_str(str, sizeof str - 1);  /* for txt_tmp2 */
						assert(str[13] == '\0');
						strlcpy(txt_tmp2, str, sizeof(txt_tmp2));
						strlcat(db_create_staff, txt_tmp2, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						  
						rand_str(str, sizeof str - 1);  /* for txt_tmp3 */
						assert(str[13] == '\0');
						strlcpy(txt_tmp3, str, sizeof(txt_tmp3));
						strlcat(db_create_staff, txt_tmp3, sizeof(db_create_staff));
						strlcat(db_create_staff, "', ", sizeof(db_create_staff));
						strlcat(db_create_staff, "'", sizeof(db_create_staff));
						  
						rand_str(str, sizeof str - 1);  /* for txt_tmp4 */
						assert(str[13] == '\0');
						strlcpy(txt_tmp4, str, sizeof(txt_tmp4));
						strlcat(db_create_staff, txt_tmp4, sizeof(db_create_staff));
						strlcat(db_create_staff, "')", sizeof(db_create_staff));
						  
						// db_search_staff + aemail
						strlcat(db_search_staff, "'", sizeof(db_search_staff));
						strlcat(db_search_staff, aemail, sizeof(db_search_staff));
						strlcat(db_search_staff, "'", sizeof(db_search_staff));
						  
						// db_set_confirm + aemail
						strlcat(db_set_confirm, "'", sizeof(db_set_confirm));
						strlcat(db_set_confirm, aemail, sizeof(db_set_confirm));
						strlcat(db_set_confirm, "'", sizeof(db_set_confirm));
						  
						// db_set_confirm_timestamp + aemail
						strlcat(db_set_confirm_timestamp, "'", sizeof(db_set_confirm_timestamp));
						strlcat(db_set_confirm_timestamp, aemail, sizeof(db_set_confirm_timestamp));
						strlcat(db_set_confirm_timestamp, "'", sizeof(db_set_confirm_timestamp));
						  
						db_exec_create = PQexec(conn, db_create_staff);
						tmp_new_email++;
						n++;
					} else {
						khttp_puts(&r, "<p>");
						khttp_puts(&r, oemail);
						khttp_puts(&r, " already exists</p>");
						tmp_dup_email++;
					}
				}
				fclose(sfp);
			}
			// Convert int to text for khttp_puts.
			itobase10(txt_new_email, tmp_new_email);
			itobase10(txt_dup_email, tmp_dup_email);
			// Only print the number imported, if it's not zero.
			if (tmp_new_email != 0) {
				khttp_puts(&r, "<p>");
				khttp_puts(&r, txt_new_email);
				khttp_puts(&r, " new accounts were imported.");
				khttp_puts(&r, "</p>");
			}
			// Only print the number NOT imported, if it's not zero.
			if (tmp_dup_email != 0) {
				khttp_puts(&r, "<p>");
				khttp_puts(&r, txt_dup_email);
				khttp_puts(&r, " accounts already exit and were not imported.");
				khttp_puts(&r, "</p>");
			}
		}
	}
	 
	khttp_puts(&r, "</div>");	// div main end
	khttp_puts(&r, "</div>");
	khttp_puts(&r, "</body>");
	khttp_puts(&r, "</html>");
	 
//TAIL
	PQclear(db_exec_search);
	PQclear(db_exec_create);
	PQclear(db_exec_confirm_timestamp);
	 
	PQclear(db_res_staff_from_email);
	PQclear(db_res_staff_from_email_sha1);
//TAIL END
	 
	PQfinish(conn); // Close connection
	khttp_free(&r);
	return (EXIT_SUCCESS);
}	// end main
