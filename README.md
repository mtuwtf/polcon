\# polcon
Policy Confirmation - do it yourself compliance apps using BCHS and KCGI

I created this project to make policy confirmation for staff easier. They would get an email with a link to click and confirm that they have "read, accept and acknowledge" whatever policy we want them to know and follow for compliance and audit purpose. I use YAMM - "Yet Another Mail Merge" to create the emails.

There are a lot of parts and dependencies to set up. I'll try to be as complete as possible at the expense of being too verbose.

This work would not have been possible without the amazing work by Kristaps Dzonsons, the author of kcgi - https://kristaps.bsd.lv/kcgi/. Kristaps is brilliant! I admire and appreciate all of his work. Anyone using my project's code would be best served by reading about the BCHS STACK - https://learnbchs.org/.

This project will assume that you are going to run this on OpenBSD using the default web server, httpd. It is not a must but I strongly recommend doing so for reasons that I will not go into here. The database that I use is PostgreSQL and this project assumes that you will be using Postgres as well. We will install PostgreSQL server on the same server that will be running httpd. I would recommend separating the database server from the web servers. However, for simpliciy, the configuration below will assume you are running everything on one server.

Install the latest release of OpenBSD. As of this writing that would be OpenBSD 6.7. I run all most of my systems on the AMD64 architecture. You will need to install the following stable packages:

libpqxx-4.0.1p2        <-- C++ client API for PostgreSQL
kcgi-0.12.0p0          <-- minimal CGI library for web applications
postgresql-client-12.2 <-- PostgreSQL RDBMS (client)
postgresql-server-12.2 <-- PostgreSQL RDBMS (server)

--- PostgreSQL configuration ---

\# su - _postgresql
$ id
uid=503(_postgresql) gid=503(_postgresql) groups=503(_postgresql)
$ ls
$ pwd
/var/postgresql
$ mkdir data
$ initdb -D /var/postgresql/data -U postgres -A md5 -W
...
Enter new superuser password:
Enter it again:
...
Success. You can now start the database server using:

    pg_ctl -D /var/postgresql/data -l logfile start

$ pg_ctl -D /var/postgresql/data -l logfile start
waiting for server to start.... done
server started
$ ls -l
total 8
drwx------  19 _postgresql  _postgresql  1024 Jul 20 10:35 data
-rw-------   1 _postgresql  _postgresql   644 Jul 20 10:35 logfile
$ createdb staffdb -U postgres
Password:
$
$ psql staffdb -U postgres
Password for user postgres:
psql (12.2)
Type "help" for help.

staffdb=# CREATE TABLE tbl_staff (
    int_staff_id  serial PRIMARY KEY,
    txt_staff_guid character varying(64),
    txt_staff_guid_sha1 character varying(64),
    txt_staff_email character varying(128),
    txt_staff_email_sha1 character varying(64),
    txt_created timestamp without time zone NOT NULL,
    txt_isp_confirmed timestamp without time zone,
    txt_coc_confirmed timestamp without time zone,
    txt_dsp_confirmed timestamp without time zone,
    txt_gdpr_confirmed timestamp without time zone,
    txt_tmp1_confirmed timestamp without time zone,
    txt_tmp2_confirmed timestamp without time zone,
    txt_tmp3_confirmed timestamp without time zone,
    txt_tmp4_confirmed timestamp without time zone,
    txt_isp character varying(64),
    txt_coc character varying(64),
    txt_dsp character varying(64),
    txt_gdpr character varying(64),
    txt_tmp1 character varying(64),
    txt_tmp2 character varying(64),
    txt_tmp3 character varying(64),
    txt_tmp4 character varying(64)
);
CREATE TABLE
staffdb=# ALTER TABLE tbl_staff OWNER TO postgres;
ALTER TABLE
staffdb=#
staffdb=# CREATE USER web WITH ENCRYPTED PASSWORD '*websecretpassword*';
CREATE ROLE
staffdb=#
staffdb=# grant connect on database staffdb to web;
GRANT
staffdb=# grant usage on schema public to web;
GRANT
staffdb=# GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO web;
GRANT
staffdb=# GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO web;
GRANT

==

...now as a superuser

\# cd /var/postgresql/data
edit pg_hba.conf
...
\# IPv4 local connections:
host     staffdb          postgres        127.0.0.1/32        md5
host     staffdb          web.            127.0.0.1/32        md5

and also edit postgresql.conf
\# - Connection Settings -
listen_addresses = '127.0.0.1'

then reload postgresql

\# rcctl reload postgresql

--- changes specific to your environment ---

Checkout this repository: git@github.com:mtuwtf/polcon.git
[m] polcon $ tree
.
├── LICENSE
├── README.md
├── db
│   └── tbl_staff.sql
├── import
│   ├── Makefile
│   └── src
│       ├── Makefile
│       ├── build.sh
│       ├── import.c
│       ├── import.html
│       └── make.sh
├── polcon.css
├── polcon.h
└── policy
    ├── Makefile
    └── src
        ├── Makefile
        ├── build.sh
        ├── make.sh
        ├── policy.c
        └── policy.html

5 directories, 17 files

doas cp polcon.h /usr/include/
doas chown root:bin /usr/include/polcon.h
doas chmod 444 /usr/include/polcon.h

doas cp polcon.css /var/www/htdocs/
doas chmod 444 /var/www/htdocs/polcon.css
doas chown www:www /var/www/htdocs/polcon.css

doas touch /var/www/cgi-bin/staff.txt && doas chmod 644 /var/www/cgi-bin/staff.txt && doas chown root:www /var/www/cgi-bin/staff.txt

Here is some dummy data or populate staff.txt with your own list of email addresses, one email per line.
cat staff.txt
yFsA5MNpfbnrj@mtu.wtf
MhubWRuWtJs3W@mtu.wtf
Ag4hsNCP9mPSJ@mtu.wtf
KvZgR4z5nK9UM@mtu.wtf
v56RAWSB5NGRF@mtu.wtf

Note for the above, if you have email addresses longer than 49 characters, there is a section in import/src/import.c that will need to be changed:
        /*
         * This may need to be increased depending
         * on the longest email in your list.
         */
        while (fscanf(sfp, "%50s", emails[n]) == 1) {

cd import/src && chmod 700 *.sh && cd -
cd policy/src && chmod 700 *.sh && cd -

There are some changes that you will need to make in import/src/import.c
Find and update the following sections in import/src/import.c
        /*
         * CHANGE THIS HERE
         */
        char txtDomain[512]  = "www.example.com";
                                ^^^^^^^^^^^^^^^ replace this with your own web server's FQDN.
         /*
         * You can add some 5500 emails with emails[5500][5500]
         * but beyond this you will need to increase system limits.
         * Tested on OpenBSD 6.7 amd64.
         */
        char    emails[500][500]; // LIMIT - be careful here. over 500 records will crash.
 
         /*
          * CHANGE THIS HERE
          */
         conninfo = "host=192.168.8.88 dbname=staffdb user=web password=CHANGEME";
                          ^^^^^^^^^^^^ should be changed to 127.0.0.1
 
===

This doc is still a WIP.
