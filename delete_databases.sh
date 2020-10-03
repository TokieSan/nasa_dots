#! /bin/sh

sqlite3 -line indexing.db 'delete from INDEXING'
sqlite3 -line files.db 'delete from FILES'
