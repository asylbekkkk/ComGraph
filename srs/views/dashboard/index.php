<?php require BASE_PATH . '/views/layout/header.php'; ?>
<h1>Қош келдіңіз, <?= htmlspecialchars($user['full_name'], ENT_QUOTES, 'UTF-8') ?>!</h1>

<?php if ($user['role'] === 'admin'): ?>
    <p>Сіздің рөліңіз: <strong>admin</strong>. Барлық кестелерге толық CRUD қолжетімді.</p>
    <div class="card-grid">
        <?php foreach ($entities as $key => $cfg): ?>
            <a class="card" href="index.php?r=crud/list&entity=<?= htmlspecialchars($key, ENT_QUOTES, 'UTF-8') ?>">
                <h3><?= htmlspecialchars($cfg['label'], ENT_QUOTES, 'UTF-8') ?></h3>
                <span>CRUD ашу →</span>
            </a>
        <?php endforeach; ?>
        <a class="card" href="index.php?r=topics/moderate">
            <h3>Тақырыптарды модерациялау</h3>
            <span>Pending тізімі →</span>
        </a>
    </div>
<?php elseif ($user['role'] === 'moderator'): ?>
    <p>Сіздің рөліңіз: <strong>moderator</strong>. Мұғалімдер ұсынған тақырыптарды бекіту/қабылдамау.</p>
    <div class="card-grid">
        <a class="card" href="index.php?r=topics/moderate">
            <h3>Тақырыптарды модерациялау</h3>
            <span>Pending тізімі →</span>
        </a>
    </div>
<?php else: ?>
    <p>Сіздің рөліңіз: <strong><?= htmlspecialchars($user['role'], ENT_QUOTES, 'UTF-8') ?></strong>.</p>
    <div class="alert alert-info">
        Студент/мұғалім интерфейсі (тақырыптар тізімі, мұғалімдер тізімі, транскрипт, профиль)
        келесі кезеңде іске асырылады — Задача 1-де фокус әкімші панеліне (CRUD) бағытталған.
    </div>
<?php endif; ?>
<?php require BASE_PATH . '/views/layout/footer.php'; ?>
