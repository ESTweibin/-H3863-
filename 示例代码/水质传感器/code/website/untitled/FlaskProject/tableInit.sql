CREATE DATABASE IF NOT EXISTS my_database;
USE my_database;
-- -- -- -- -- -- -- -- -- -- -- --
-- 用户表
-- -- -- -- -- -- -- -- -- -- -- --
CREATE TABLE IF NOT EXISTS `Users` (
    `user_id` INT AUTO_INCREMENT PRIMARY KEY,
    `email` VARCHAR(255) NOT NULL UNIQUE,
    `password_hash` VARCHAR(255) NOT NULL,
    `full_name` VARCHAR(100) NULL,
    `role` VARCHAR(50) NULL DEFAULT 'admin' COMMENT '例如: admin, researcher, operator',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户信息表';

-- -- -- -- -- -- -- -- -- -- -- --
-- 航行记录表
-- -- -- -- -- -- -- -- -- -- -- --
CREATE TABLE IF NOT EXISTS `Voyages` (
    `voyage_id` INT AUTO_INCREMENT PRIMARY KEY,
    `user_id` INT NULL COMMENT '执行航行的用户ID',
    `vessel_identifier` VARCHAR(100) NULL COMMENT '船只/设备标识 (例如无人机ID, 船名)',
    `start_timestamp` DATETIME NOT NULL COMMENT '航行开始时间',
    `end_timestamp` DATETIME NULL COMMENT '航行结束时间',
    `purpose` TEXT NULL COMMENT '航行目的',
    `notes` TEXT NULL COMMENT '备注',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (`user_id`) REFERENCES `Users`(`user_id`) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='航行记录表';

-- -- -- -- -- -- -- -- -- -- -- --
-- 航行路径点表
-- -- -- -- -- -- -- -- -- -- -- --
CREATE TABLE IF NOT EXISTS `VoyagePathPoints` (
    `point_id` BIGINT AUTO_INCREMENT PRIMARY KEY,
    `voyage_id` INT NOT NULL COMMENT '关联的航行ID',
    `latitude` DECIMAL(10, 8) NOT NULL COMMENT '纬度',
    `longitude` DECIMAL(11, 8) NOT NULL COMMENT '经度',
    `altitude` DECIMAL(7, 2) NULL COMMENT '海拔高度 (例如米)',
    `speed` DECIMAL(5, 2) NULL COMMENT '速度 (例如节, 米/秒)',
    `timestamp` DATETIME NOT NULL COMMENT '该点的记录时间',
    FOREIGN KEY (`voyage_id`) REFERENCES `Voyages`(`voyage_id`) ON DELETE CASCADE,
    INDEX `idx_voyage_path_points_voyage_id_timestamp` (`voyage_id`, `timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='航行路径的GPS轨迹点';

-- -- -- -- -- -- -- -- -- -- -- --
-- 水质信息表
-- -- -- -- -- -- -- -- -- -- -- --
CREATE TABLE IF NOT EXISTS `WaterQualityReadings` (
    `reading_id` INT AUTO_INCREMENT PRIMARY KEY,
    `latitude` DECIMAL(10, 8) NOT NULL COMMENT '实际采样点纬度',
    `longitude` DECIMAL(11, 8) NOT NULL COMMENT '实际采样点经度',
    `timestamp` DATETIME NOT NULL COMMENT '采样时间',
    `temperature_celsius` DECIMAL(5, 2) NULL COMMENT '温度 (摄氏度)',
    `ph_value` DECIMAL(4, 2) NULL COMMENT 'pH值 (0-14)',
    `dissolved_oxygen_mgl` DECIMAL(5, 2) NULL COMMENT '溶解氧 (mg/L)',
    `turbidity_ntu` DECIMAL(6, 2) NULL COMMENT '浊度 (NTU)',
    `conductivity_us_cm` DECIMAL(7, 2) NULL COMMENT '电导率 (µS/cm)',
    `source` VARCHAR(50) DEFAULT 'manual' COMMENT '数据来源 (例如: manual, sensor_A, drone_sensor)',
    `notes` TEXT NULL COMMENT '备注',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX `idx_wq_timestamp` (`timestamp`),
    INDEX `idx_wq_geolocation` (`latitude`, `longitude`),
    CHECK (`ph_value` >= 0 AND `ph_value` <= 14 OR `ph_value` IS NULL)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='水质监测数据表';

-- -- -- -- -- -- -- -- -- -- -- --
-- 福寿螺观测记录表
-- -- -- -- -- -- -- -- -- -- -- --
CREATE TABLE IF NOT EXISTS `SnailObservations` (
    `observation_id` INT AUTO_INCREMENT PRIMARY KEY,
    `latitude` DECIMAL(10, 8) NOT NULL COMMENT '观测点纬度',
    `longitude` DECIMAL(11, 8) NOT NULL COMMENT '观测点经度',
    `timestamp` DATETIME NOT NULL COMMENT '观测时间',
    `apple_snail_count` INT NOT NULL DEFAULT 0 COMMENT '福寿螺数量',
    `notes` TEXT NULL COMMENT '备注',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX `idx_so_timestamp` (`timestamp`),
    INDEX `idx_so_geolocation` (`latitude`, `longitude`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='福寿螺观测记录表';

-- -- -- -- -- -- -- -- -- -- -- --
-- 治理记录表
-- -- -- -- -- -- -- -- -- -- -- --
CREATE TABLE IF NOT EXISTS `TreatmentRecords` (
    `treatment_record_id` INT AUTO_INCREMENT PRIMARY KEY,
    `treatment_date` DATE NOT NULL COMMENT '治理实施日期',
    `count_before_treatment` INT NOT NULL COMMENT '治理前福寿螺数量',
    `date_of_count_before` DATE NOT NULL COMMENT '治理前数量的统计日期',
    `count_after_treatment` INT NULL COMMENT '治理后福寿螺数量',
    `date_of_count_after` DATE NULL COMMENT '治理后数量的统计日期',
    `efficacy_notes` TEXT NULL COMMENT '效果评估备注',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='福寿螺治理活动记录表';

-- -- -- -- -- -- -- -- -- -- -- --
-- 警报表
-- -- -- -- -- -- -- -- -- -- -- --
CREATE TABLE IF NOT EXISTS `Alerts` (
    `alert_id` INT AUTO_INCREMENT PRIMARY KEY,
    `alert_type` VARCHAR(100) NOT NULL COMMENT '警报类型 (例如: HIGH_SNAIL_DENSITY, LOW_PH)',
    `severity` ENUM('LOW', 'MEDIUM', 'HIGH', 'CRITICAL') NOT NULL DEFAULT 'MEDIUM' COMMENT '警报级别',
    `snail_observation_id` INT NULL COMMENT '关联的福寿螺观测ID (如果警报来源于此)',
    `water_quality_reading_id` INT NULL COMMENT '关联的水质读数ID (如果警报来源于此)',
    `latitude` DECIMAL(10, 8) NOT NULL COMMENT '警报发生点纬度 (冗余存储, 便于查询)',
    `longitude` DECIMAL(11, 8) NOT NULL COMMENT '警报发生点经度 (冗余存储, 便于查询)',
    `alert_timestamp` DATETIME NOT NULL COMMENT '警报发生/报告时间',
    `description` TEXT NULL COMMENT '警报详细描述',
    `status` ENUM('NEW', 'INVESTIGATING', 'ACTION_TAKEN', 'RESOLVED', 'FALSE_ALARM') NOT NULL DEFAULT 'NEW' COMMENT '警报状态',
    `resolved_by_user_id` INT NULL COMMENT '解决警报的用户ID',
    `resolved_at` DATETIME NULL COMMENT '警报解决时间',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (`snail_observation_id`) REFERENCES `SnailObservations`(`observation_id`) ON DELETE SET NULL,
    FOREIGN KEY (`water_quality_reading_id`) REFERENCES `WaterQualityReadings`(`reading_id`) ON DELETE SET NULL,
    FOREIGN KEY (`resolved_by_user_id`) REFERENCES `Users`(`user_id`) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='异常警报信息表';