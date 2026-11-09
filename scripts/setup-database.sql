\set ON_ERROR_STOP on
SET client_encoding = 'UTF8';

-- Administrative infrastructure only. Application tables are developed in later TDD cycles.
DO $setup$
BEGIN
    IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'habit_tracker_tdd_app'
               AND (rolsuper OR rolcreatedb OR rolcreaterole OR rolreplication OR rolbypassrls)) THEN
        RAISE EXCEPTION 'Existing TDD role has administrative privileges; setup stopped.';
    END IF;
    IF EXISTS (SELECT 1 FROM pg_database d JOIN pg_roles r ON r.oid = d.datdba
               WHERE d.datname IN ('habit_tracker_tdd', 'habit_tracker_tdd_test')
                 AND r.rolname <> 'habit_tracker_tdd_app') THEN
        RAISE EXCEPTION 'An existing TDD database has another owner; setup stopped.';
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'habit_tracker_tdd_app') THEN
        CREATE ROLE habit_tracker_tdd_app LOGIN NOSUPERUSER NOCREATEDB NOCREATEROLE NOREPLICATION NOBYPASSRLS;
    END IF;
END
$setup$;

-- psql handles hidden input and avoids writing the cleartext password to a SQL file.
\password habit_tracker_tdd_app

SELECT format('CREATE DATABASE %I OWNER habit_tracker_tdd_app TEMPLATE template0 ENCODING %L', name, 'UTF8')
FROM (VALUES ('habit_tracker_tdd'), ('habit_tracker_tdd_test')) AS requested(name)
WHERE NOT EXISTS (SELECT 1 FROM pg_database WHERE datname = requested.name)
\gexec

REVOKE ALL ON DATABASE habit_tracker_tdd FROM PUBLIC;
REVOKE ALL ON DATABASE habit_tracker_tdd_test FROM PUBLIC;
SELECT datname, pg_get_userbyid(datdba) AS owner
FROM pg_database WHERE datname IN ('habit_tracker_tdd', 'habit_tracker_tdd_test') ORDER BY datname;
