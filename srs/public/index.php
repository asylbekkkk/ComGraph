<?php
declare(strict_types=1);

require_once dirname(__DIR__) . '/config/config.php';

/**
 * Простой роутер: index.php?r=controller/action&...
 * Собственный MVC без фреймворка, как того требует задание.
 */
$route = $_GET['r'] ?? 'auth/login';
[$seg1, $seg2] = array_pad(explode('/', $route, 2), 2, 'index');

$routes = [
    'auth' => [
        'login'  => ['AuthController', 'GET' === $_SERVER['REQUEST_METHOD'] ? 'loginForm' : 'login'],
        'logout' => ['AuthController', 'logout'],
    ],
    'dashboard' => [
        'index' => ['DashboardController', 'index'],
    ],
    'crud' => [
        'list'   => ['CrudController', 'list'],
        'create' => ['CrudController', 'GET' === $_SERVER['REQUEST_METHOD'] ? 'createForm' : 'save'],
        'edit'   => ['CrudController', 'GET' === $_SERVER['REQUEST_METHOD'] ? 'editForm' : 'save'],
        'delete' => ['CrudController', 'delete'],
    ],
    'topics' => [
        'moderate' => ['TopicsController', 'moderate'],
        'decide'   => ['TopicsController', 'decide'],
    ],
];

if (!isset($routes[$seg1][$seg2])) {
    http_response_code(404);
    echo 'Страница не найдена. <a href="index.php">На главную</a>';
    exit;
}

[$controllerName, $method] = $routes[$seg1][$seg2];
$controller = new $controllerName();
$controller->$method();
