<?php
declare(strict_types=1);

/**
 * Database — тонкая обёртка над PDO (singleton).
 * Все запросы — только через prepared statements, без конкатенации строк.
 */
class Database
{
    private static ?PDO $instance = null;

    public static function connection(): PDO
    {
        if (self::$instance === null) {
            $dsn = 'mysql:host=' . DB_HOST . ';dbname=' . DB_NAME . ';charset=' . DB_CHARSET;
            try {
                self::$instance = new PDO($dsn, DB_USER, DB_PASS, [
                    PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
                    PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
                    PDO::ATTR_EMULATE_PREPARES   => false, // настоящие prepared statements
                ]);
            } catch (PDOException $e) {
                http_response_code(500);
                die('Ошибка подключения к БД. Проверьте config/config.php и что MySQL запущен в XAMPP.');
            }
        }
        return self::$instance;
    }
}
