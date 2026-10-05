<?php
/**
 * config/config.php
 * Общий bootstrap: настройки БД, сессия, автозагрузка классов.
 * Подключается из public/index.php (единая точка входа).
 */

declare(strict_types=1);

// --- Настройки БД (XAMPP по умолчанию) ---------------------------------
define('DB_HOST', 'localhost');
define('DB_NAME', 'diploma_topics');
define('DB_USER', 'root');
define('DB_PASS', '');          // в XAMPP по умолчанию пустой пароль root
define('DB_CHARSET', 'utf8mb4');

// --- Базовый путь проекта (для ссылок вида BASE_URL . '/index.php?...') -
define('BASE_PATH', dirname(__DIR__));

// --- Безопасная сессия ---------------------------------------------------
if (session_status() === PHP_SESSION_NONE) {
    session_set_cookie_params([
        'lifetime' => 0,
        'path'     => '/',
        'httponly' => true,   // недоступно из JS -> защита от XSS-кражи cookie
        'samesite' => 'Lax',
    ]);
    session_start();
}

// --- Вывод ошибок (только для разработки! выключить в проде) -----------
ini_set('display_errors', '1');
error_reporting(E_ALL);

// --- Простая автозагрузка классов из src/ -------------------------------
spl_autoload_register(function (string $class): void {
    // Классы лежат плоско в src/, без namespace, имя файла = имя класса
    $file = BASE_PATH . '/src/' . $class . '.php';
    if (is_file($file)) {
        require_once $file;
    }
});

require_once BASE_PATH . '/config/entities.php';
