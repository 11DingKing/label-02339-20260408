-- 环境监测系统 - SQLite 数据库建表脚本
-- 使用方式: 程序启动时自动执行（DatabaseManager::createTable）

CREATE TABLE IF NOT EXISTS environment_data (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    collect_time    TEXT    NOT NULL,   -- 采集时间 格式: yyyy-MM-dd HH:mm:ss
    temperature     REAL    NOT NULL,   -- 温度 ℃
    humidity        REAL    NOT NULL,   -- 湿度 %
    light_intensity REAL    NOT NULL    -- 光照强度 Lux
);

-- 时间索引，加速按时间范围查询
CREATE INDEX IF NOT EXISTS idx_collect_time ON environment_data(collect_time);
