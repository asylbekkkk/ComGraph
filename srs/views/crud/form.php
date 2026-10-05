<?php
/** @var string $name */
/** @var array $config */
/** @var CrudModel $model */
/** @var array $row */
/** @var array $errors */
/** @var array $old */
/** @var string $mode */
require BASE_PATH . '/views/layout/header.php';

$isEdit = $mode === 'edit';
$values = $old ?: $row; // при ошибке валидации показываем то, что ввёл пользователь
?>
<h1><?= $isEdit ? 'Өзгерту' : 'Жаңа жазба қосу' ?>: <?= htmlspecialchars($config['label'], ENT_QUOTES, 'UTF-8') ?></h1>

<form method="post" action="index.php?r=crud/<?= $isEdit ? 'edit' : 'create' ?>&entity=<?= htmlspecialchars($name, ENT_QUOTES, 'UTF-8') ?>" class="entity-form">
    <?= Csrf::field() ?>
    <input type="hidden" name="entity" value="<?= htmlspecialchars($name, ENT_QUOTES, 'UTF-8') ?>">
    <?php if ($isEdit): ?>
        <input type="hidden" name="id" value="<?= (int)$row[$config['pk']] ?>">
    <?php endif; ?>

    <?php foreach ($config['fields'] as $field => $meta): ?>
        <?php
            if ($meta['type'] === 'readonly' && !$isEdit) continue; // при создании readonly-поля (id, created_at) не показываем
            $val = $values[$field] ?? '';
            $fieldError = $errors[$field] ?? null;
        ?>
        <div class="form-row <?= $fieldError ? 'has-error' : '' ?>">
            <label for="f_<?= $field ?>">
                <?= htmlspecialchars($meta['label'], ENT_QUOTES, 'UTF-8') ?>
                <?php if (!empty($meta['required'])): ?><span class="req">*</span><?php endif; ?>
            </label>

            <?php switch ($meta['type']):
                case 'readonly': ?>
                    <input type="text" id="f_<?= $field ?>" value="<?= htmlspecialchars((string)$val, ENT_QUOTES, 'UTF-8') ?>" disabled>
                    <?php break;

                case 'textarea': ?>
                    <textarea id="f_<?= $field ?>" name="<?= $field ?>" rows="4"><?= htmlspecialchars((string)$val, ENT_QUOTES, 'UTF-8') ?></textarea>
                    <?php break;

                case 'password': ?>
                    <input type="password" id="f_<?= $field ?>" name="<?= $field ?>" autocomplete="new-password">
                    <?php if (!empty($meta['help'])): ?><small><?= htmlspecialchars($meta['help'], ENT_QUOTES, 'UTF-8') ?></small><?php endif; ?>
                    <?php break;

                case 'select': ?>
                    <select id="f_<?= $field ?>" name="<?= $field ?>">
                        <option value="">—</option>
                        <?php foreach ($meta['options'] as $optVal => $optLabel): ?>
                            <option value="<?= htmlspecialchars((string)$optVal, ENT_QUOTES, 'UTF-8') ?>" <?= (string)$val === (string)$optVal ? 'selected' : '' ?>>
                                <?= htmlspecialchars((string)$optLabel, ENT_QUOTES, 'UTF-8') ?>
                            </option>
                        <?php endforeach; ?>
                    </select>
                    <?php break;

                case 'select_fk': ?>
                    <select id="f_<?= $field ?>" name="<?= $field ?>">
                        <option value="">—</option>
                        <?php foreach ($model->fkOptions($meta) as $optVal => $optLabel): ?>
                            <option value="<?= (int)$optVal ?>" <?= (string)$val === (string)$optVal ? 'selected' : '' ?>>
                                <?= htmlspecialchars((string)$optLabel, ENT_QUOTES, 'UTF-8') ?>
                            </option>
                        <?php endforeach; ?>
                    </select>
                    <?php break;

                case 'number': ?>
                    <input type="number" step="1" id="f_<?= $field ?>" name="<?= $field ?>" value="<?= htmlspecialchars((string)$val, ENT_QUOTES, 'UTF-8') ?>">
                    <?php break;

                case 'decimal': ?>
                    <input type="number" step="0.01" id="f_<?= $field ?>" name="<?= $field ?>" value="<?= htmlspecialchars((string)$val, ENT_QUOTES, 'UTF-8') ?>">
                    <?php break;

                default: ?>
                    <input type="text" id="f_<?= $field ?>" name="<?= $field ?>" value="<?= htmlspecialchars((string)$val, ENT_QUOTES, 'UTF-8') ?>">
            <?php endswitch; ?>

            <?php if ($fieldError): ?><div class="field-error"><?= htmlspecialchars($fieldError, ENT_QUOTES, 'UTF-8') ?></div><?php endif; ?>
        </div>
    <?php endforeach; ?>

    <div class="form-actions">
        <button type="submit" class="btn btn-primary">Сақтау</button>
        <a class="btn" href="index.php?r=crud/list&entity=<?= htmlspecialchars($name, ENT_QUOTES, 'UTF-8') ?>">Бас тарту</a>
    </div>
</form>

<?php require BASE_PATH . '/views/layout/footer.php'; ?>
