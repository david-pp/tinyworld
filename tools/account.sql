CREATE TABLE IF NOT EXISTS `account` (
	`accid` INT(10) UNSIGNED NOT NULL auto_increment,
	`username` VARCHAR(64) NOT NULL default '',
	`passwd` VARCHAR(128) NOT NULL default '',
	`device_type` VARCHAR(64) NOT NULL default '',
	`device_id` VARCHAR(64) NOT NULL default '',
	PRIMARY KEY (`accid`)
) ENGINE = InnoDB;