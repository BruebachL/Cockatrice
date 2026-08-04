-- Servatrice db migration from version 36 to version 37

ALTER TABLE `cockatrice_users` MODIFY `password_sha512` char(255) NOT NULL, ALGORITHM=INSTANT;

UPDATE cockatrice_schema_version SET version=37 WHERE version=36;