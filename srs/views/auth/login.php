<?php require BASE_PATH . '/views/layout/header.php'; ?>
<div class="login-box">
    <h1>Кіру</h1>
    <?php if (!empty($error)): ?>
        <div class="alert alert-error"><?= htmlspecialchars($error, ENT_QUOTES, 'UTF-8') ?></div>
    <?php endif; ?>
    <form method="post" action="index.php?r=auth/login">
        <?= Csrf::field() ?>
        <label>Логин
            <input type="text" name="login" required autofocus>
        </label>
        <label>Пароль
            <input type="password" name="password" required>
        </label>
        <button type="submit" class="btn btn-primary">Кіру</button>
    </form>
    <p class="hint">Демо: admin / moderator1 / teacher1 / student1 — пароль <code>Password123!</code></p>
</div>
<?php require BASE_PATH . '/views/layout/footer.php'; ?>
