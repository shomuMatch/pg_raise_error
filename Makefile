MODULES = pg_raise_error
EXTENSION = pg_raise_error
DATA = pg_raise_error--1.0.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
