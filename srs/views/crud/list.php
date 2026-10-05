<?php
/** @var string $name */
/** @var array $config */
/** @var array $rows */
/** @var int $page */
/** @var int $totalPages */
/** @var int $total */
require BASE_PATH . '/views/layout/header.php';
?>
<div class="list-header">
    <h1><?= htmlspecialchars($config['label'], ENT_QUOTES, 'UTF-8') ?></h1>
    <a class="btn btn-primary" href="index.php?r=crud/create&entity=<?= htmlspecialchars($name, ENT_QUOTES, 'UTF-8') ?>">+ Қосу</a>
</div>

<?php if (!empty($_SESSION['flash_error'])): ?>
    <div class="alert alert-error"><?= htmlspecialchars($_SESSION['flash_error'], ENT_QUOTES, 'UTF-8') ?></div>
    <?php unset($_SESSION['flash_error']); ?>
<?php endif; ?>

<div class="table-wrap">
<table class="data-table">
<thead>
<tr>
    <?php foreach ($config['list_columns'] as $col): ?>
        <th><?= htmlspecialchars($config['fields'][$col]['label'] ?? $col, ENT_QUOTES, 'UTF-8') ?></th>
    <?php endforeach; ?>
    <th>Әрекеттер</th>
</tr>
</thead>
<tbody>
<?php if (empty($rows)): ?>
    <tr><td colspan="<?= count($config['list_columns']) + 1 ?>" class="empty">Жазба жоқ.</td></tr>
<?php endif; ?>
<?php foreach ($rows as $row): ?>
    <tr>
        <?php foreach ($config['list_columns'] as $col): ?>
            <td><?= htmlspecialchars((string)($row[$col] ?? ''), ENT_QUOTES, 'UTF-8') ?></td>
        <?php endforeach; ?>
        <td class="actions">
            <a class="btn btn-sm" href="index.php?r=crud/edit&entity=<?= htmlspecialchars($name, ENT_QUOTES, 'UTF-8') ?>&id=<?= (int)$row[$config['pk']] ?>">Өзгерту</a>
            <form method="post" action="index.php?r=crud/delete&entity=<?= htmlspecialchars($name, ENT_QUOTES, 'UTF-8') ?>"
                  onsubmit="return confirm('Жазбаны шынымен жоясыз ба?');" class="inline-form">
                <?= Csrf::field() ?>
                <input type="hidden" name="id" value="<?= (int)$row[$config['pk']] ?>">
                <button type="submit" class="btn btn-sm btn-danger">Жою</button>
            </form>
        </td>
    </tr>
<?php endforeach; ?>
</tbody>
</table>
</div>

<div class="pagination">
    <?php for ($p = 1; $p <= $totalPages; $p++): ?>
        <a class="<?= $p === $page ? 'active' : '' ?>"
           href="index.php?r=crud/list&entity=<?= htmlspecialchars($name, ENT_QUOTES, 'UTF-8') ?>&page=<?= $p ?>"><?= $p ?></a>
    <?php endfor; ?>
    <span class="total">Барлығы: <?= (int)$total ?></span>
</div>

<?php require BASE_PATH . '/views/layout/footer.php'; ?>
