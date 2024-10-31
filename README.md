# pg_raise_error

PostgreSQL extension to raise error after executions of query of specified number.

## Instllation

```bash
$ make && make install

$ vi PATH/TO/CONF/postgresql.conf
# add this
shared_preload_libraries = 'pg_raise_error'

$ psql
# CREATE EXTENSION pg_raise_error;
```

## Usage

```sql
-- set error reservation
SELECT set_error_trigger(10, 'error');
-- the error below occurs after 10 queries
-- ERROR: COUNT IS ZERO!!

-- clear all error reservations
SELECT clear_error_trigger();
```
