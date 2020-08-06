# polcon
Policy Confirmation - do it yourself compliance apps using BCHS and KCGI

I created this project to make policy confirmation for staff easier. They would get an email with a link to click and confirm that they have "read, accept and acknowledge" whatever policy we want them to know and follow for compliance and audit purpose. I use YAMM - "Yet Another Mail Merge" to create the emails.

There are a lot of parts and dependencies to set up. I'll try to be as complete as possible at the expense of being too verbose.

This work would not have been possible without the amazing work by Kristaps Dzonsons, the author of kcgi - https://kristaps.bsd.lv/kcgi/. Kristaps is brilliant! I admire and appreciate all of his work. Anyone using my project's code would be best served by reading about the BCHS STACK - https://learnbchs.org/.

This project will assume that you are going to run this on OpenBSD using the default web server, httpd. It is not a must but I strongly recommended doing so for reasons that I will not go into here. The database that I use is PostgreSQL and this project assumes that you will be using Postgres as well. 

This doc is still a WIP.
