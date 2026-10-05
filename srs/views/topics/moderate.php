<?php
/** @var array $pendingTopics */
require BASE_PATH . '/views/layout/header.php';
?>
<h1>Тақырыптарды модерациялау</h1>
<p>Мұғалімдер ұсынған, әлі қаралмаған (<strong>pending</strong>) тақырыптар:</p>

<?php if (empty($pendingTopics)): ?>
    <div class="alert alert-info">Қаралуы тиіс тақырыптар жоқ.</div>
<?php endif; ?>

<div class="topic-cards">
<?php foreach ($pendingTopics as $t): ?>
    <div class="topic-card">
        <h3><?= htmlspecialchars($t['title'], ENT_QUOTES, 'UTF-8') ?></h3>
        <p class="teacher">Мұғалім: <?= htmlspecialchars($t['teacher_name'], ENT_QUOTES, 'UTF-8') ?></p>
        <p><?= nl2br(htmlspecialchars($t['description'] ?? '', ENT_QUOTES, 'UTF-8')) ?></p>
        <div class="topic-actions">
            <form method="post" action="index.php?r=topics/decide" class="inline-form">
                <?= Csrf::field() ?>
                <input type="hidden" name="id" value="<?= (int)$t['id'] ?>">
                <input type="hidden" name="decision" value="approved">
                <button type="submit" class="btn btn-primary btn-sm">✓ Бекіту</button>
            </form>
            <form method="post" action="index.php?r=topics/decide" class="inline-form">
                <?= Csrf::field() ?>
                <input type="hidden" name="id" value="<?= (int)$t['id'] ?>">
                <input type="hidden" name="decision" value="rejected">
                <button type="submit" class="btn btn-danger btn-sm">✕ Қабылдамау</button>
            </form>
        </div>
    </div>
<?php endforeach; ?>
</div>

<?php require BASE_PATH . '/views/layout/footer.php'; ?>
