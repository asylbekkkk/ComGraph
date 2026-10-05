<?php
declare(strict_types=1);

/**
 * Auth — аутентификация и проверка ролей.
 * ВАЖНО: проверка роли выполняется на сервере (requireRole), а не только
 * скрытием ссылок в UI.
 */
class Auth
{
    public static function attempt(string $login, string $password): bool
    {
        $pdo = Database::connection();
        $stmt = $pdo->prepare('SELECT id, login, password_hash, role, full_name FROM users WHERE login = :login LIMIT 1');
        $stmt->execute(['login' => $login]);
        $user = $stmt->fetch();

        if (!$user || !password_verify($password, $user['password_hash'])) {
            return false;
        }

        // защита от session fixation
        session_regenerate_id(true);

        $_SESSION['user'] = [
            'id'        => (int)$user['id'],
            'login'     => $user['login'],
            'role'      => $user['role'],
            'full_name' => $user['full_name'],
        ];
        return true;
    }

    public static function logout(): void
    {
        $_SESSION = [];
        if (ini_get('session.use_cookies')) {
            $params = session_get_cookie_params();
            setcookie('PHPSESSID', '', time() - 42000, $params['path']);
        }
        session_destroy();
    }

    public static function check(): bool
    {
        return isset($_SESSION['user']);
    }

    public static function user(): ?array
    {
        return $_SESSION['user'] ?? null;
    }

    public static function role(): ?string
    {
        return $_SESSION['user']['role'] ?? null;
    }

    /** Требует, чтобы пользователь был залогинен. Иначе — редирект на логин. */
    public static function requireLogin(): void
    {
        if (!self::check()) {
            header('Location: index.php?r=auth/login');
            exit;
        }
    }

    /**
     * Требует одну из перечисленных ролей. Проверка ВСЕГДА на сервере.
     * @param string[] $roles
     */
    public static function requireRole(array $roles): void
    {
        self::requireLogin();
        if (!in_array(self::role(), $roles, true)) {
            http_response_code(403);
            require BASE_PATH . '/views/errors/403.php';
            exit;
        }
    }
}
