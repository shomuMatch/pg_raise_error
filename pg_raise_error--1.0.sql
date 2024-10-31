CREATE TABLE error_triggers (count int, level text);

CREATE FUNCTION set_error_trigger(int, text) RETURNS bool AS 'MODULE_PATHNAME' LANGUAGE C;

CREATE FUNCTION clear_error_trigger() RETURNS bool AS 'MODULE_PATHNAME' LANGUAGE C;
