<?php
declare(strict_types=1);

/**
 * Csrf — генерация и проверка CSRF-токенов для всех форм админки.
 */
class Csrf
{
    public static function token(): string
    {
        if (empty($_SESSION['csrf_token'])) {
            $_SESSION['csrf_token'] = bin2hex(random_bytes(32));
        }
        return $_SESSION['csrf_token'];
    }

    /** <input type="hidden"> с токеном — вставлять в каждую форму */
    public static function field(): string
    {
        $t = htmlspecialchars(self::token(), ENT_QUOTES, 'UTF-8');
        return '<input type="hidden" name="csrf_token" value="' . $t . '">';
    }

    public static function verify(?string $token): bool
    {
        return is_string($token) && !empty($_SESSION['csrf_token'])
            && hash_equals($_SESSION['csrf_token'], $token);
    }

    /** Проверить токен из $_POST и завершить с 403 при неудаче */
    public static function verifyOrFail(): void
    {
        if (!self::verify($_POST['csrf_token'] ?? null)) {
            http_response_code(403);
            die('Ошибка CSRF-токена. Обновите страницу и попробуйте снова.');
        }
    }
}
