<?php
declare(strict_types=1);

class AuthController
{
    public function loginForm(): void
    {
        if (Auth::check()) {
            header('Location: index.php?r=dashboard');
            exit;
        }
        $error = $_SESSION['flash_error'] ?? null;
        unset($_SESSION['flash_error']);
        require BASE_PATH . '/views/auth/login.php';
    }

    public function login(): void
    {
        Csrf::verifyOrFail();

        $login = trim((string)($_POST['login'] ?? ''));
        $password = (string)($_POST['password'] ?? '');

        if ($login === '' || $password === '') {
            $_SESSION['flash_error'] = 'Введите логин и пароль.';
            header('Location: index.php?r=auth/login');
            exit;
        }

        if (Auth::attempt($login, $password)) {
            header('Location: index.php?r=dashboard');
            exit;
        }

        $_SESSION['flash_error'] = 'Неверный логин или пароль.';
        header('Location: index.php?r=auth/login');
        exit;
    }

    public function logout(): void
    {
        Auth::logout();
        header('Location: index.php?r=auth/login');
        exit;
    }
}
