<?php
/** @var array|null $user */
$user = Auth::user();
?>
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Дипломдық тақырыптар — Админ панель</title>
<link rel="stylesheet" href="assets/css/style.css">
</head>
<body>
<?php if ($user): ?>
<header class="topbar">
    <div class="brand">📚 Дипломдық тақырыптар</div>
    <nav class="topnav">
        <a href="index.php?r=dashboard">Басты бет</a>
        <?php if ($user['role'] === 'admin'): ?>
            <?php foreach (entities_config() as $key => $cfg): ?>
                <a href="index.php?r=crud/list&entity=<?= htmlspecialchars($key, ENT_QUOTES, 'UTF-8') ?>"><?= htmlspecialchars($cfg['label'], ENT_QUOTES, 'UTF-8') ?></a>
            <?php endforeach; ?>
        <?php endif; ?>
        <?php if (in_array($user['role'], ['moderator', 'admin'], true)): ?>
            <a href="index.php?r=topics/moderate">Модерация тем</a>
        <?php endif; ?>
    </nav>
    <div class="userbox">
        <span><?= htmlspecialchars($user['full_name'], ENT_QUOTES, 'UTF-8') ?> (<?= htmlspecialchars($user['role'], ENT_QUOTES, 'UTF-8') ?>)</span>
        <a href="index.php?r=auth/logout" class="btn btn-sm">Шығу</a>
    </div>
</header>
<?php endif; ?>
<main class="container">
