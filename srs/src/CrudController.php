<?php
declare(strict_types=1);

/**
 * CrudController — единый контроллер CRUD для всех сущностей из
 * config/entities.php. Роут: index.php?r=crud/{action}&entity={name}
 */
class CrudController
{
    private const PER_PAGE = 10;

    private function resolveEntity(): array
    {
        $name = $_GET['entity'] ?? $_POST['entity'] ?? '';
        $all = entities_config();
        if (!isset($all[$name])) {
            http_response_code(404);
            die('Неизвестная сущность.');
        }
        $config = $all[$name];
        // server-side проверка роли для этой сущности (не полагаемся на скрытие ссылок в UI)
        Auth::requireRole($config['roles'] ?? ['admin']);
        return [$name, $config];
    }

    public function list(): void
    {
        [$name, $config] = $this->resolveEntity();
        $model = new CrudModel($config);

        $page = max(1, (int)($_GET['page'] ?? 1));
        $total = $model->count();
        $totalPages = max(1, (int)ceil($total / self::PER_PAGE));
        $page = min($page, $totalPages);
        $rows = $model->paginate($page, self::PER_PAGE);

        require BASE_PATH . '/views/crud/list.php';
    }

    public function createForm(): void
    {
        [$name, $config] = $this->resolveEntity();
        $model = new CrudModel($config);
        $row = [];
        $errors = $_SESSION['form_errors'] ?? [];
        $old = $_SESSION['form_old'] ?? [];
        unset($_SESSION['form_errors'], $_SESSION['form_old']);
        $mode = 'create';
        require BASE_PATH . '/views/crud/form.php';
    }

    public function editForm(): void
    {
        [$name, $config] = $this->resolveEntity();
        $model = new CrudModel($config);
        $id = (int)($_GET['id'] ?? 0);
        $row = $model->find($id);
        if (!$row) {
            http_response_code(404);
            die('Запись не найдена.');
        }
        $errors = $_SESSION['form_errors'] ?? [];
        $old = $_SESSION['form_old'] ?? [];
        unset($_SESSION['form_errors'], $_SESSION['form_old']);
        $mode = 'edit';
        require BASE_PATH . '/views/crud/form.php';
    }

    public function save(): void
    {
        Csrf::verifyOrFail();
        [$name, $config] = $this->resolveEntity();
        $model = new CrudModel($config);

        $id = (int)($_POST['id'] ?? 0);
        $isEdit = $id > 0;

        [$data, $errors] = $this->validate($config, $_POST, $isEdit);

        if (!empty($errors)) {
            $_SESSION['form_errors'] = $errors;
            $_SESSION['form_old'] = $_POST;
            $target = $isEdit
                ? "index.php?r=crud/edit&entity=$name&id=$id"
                : "index.php?r=crud/create&entity=$name";
            header('Location: ' . $target);
            exit;
        }

        // Спец. обработка пароля для users
        if ($name === 'users') {
            $plainPassword = (string)($_POST['password'] ?? '');
            if ($plainPassword !== '') {
                $data['password_hash'] = password_hash($plainPassword, PASSWORD_BCRYPT);
            } elseif (!$isEdit) {
                // при создании пароль обязателен
                $_SESSION['form_errors'] = ['password' => 'Пароль обязателен при создании пользователя.'];
                $_SESSION['form_old'] = $_POST;
                header('Location: index.php?r=crud/create&entity=users');
                exit;
            }
        }

        // reviewed_at авто-проставляем для topics, если сменился статус модерации
        if ($name === 'topics' && $isEdit) {
            $existing = $model->find($id);
            if ($existing && $existing['status'] === 'pending' && in_array($data['status'] ?? '', ['approved', 'rejected'], true)) {
                $data['reviewed_by'] = Auth::user()['id'];
                $data['reviewed_at'] = date('Y-m-d H:i:s');
            }
        }

        if ($isEdit) {
            $model->update($id, $data);
        } else {
            $model->create($data);
        }

        header('Location: index.php?r=crud/list&entity=' . $name);
        exit;
    }

    public function delete(): void
    {
        Csrf::verifyOrFail();
        [$name, $config] = $this->resolveEntity();
        $model = new CrudModel($config);
        $id = (int)($_POST['id'] ?? 0);
        if ($id > 0) {
            try {
                $model->delete($id);
            } catch (PDOException $e) {
                // вероятно нарушение внешнего ключа — покажем понятную ошибку
                $_SESSION['flash_error'] = 'Не удалось удалить запись: на неё ссылаются другие таблицы.';
            }
        }
        header('Location: index.php?r=crud/list&entity=' . $name);
        exit;
    }

    /**
     * Серверная валидация формы по конфигу полей (белый список + required + типы).
     * @return array{0: array, 1: array} [очищенные данные, ошибки]
     */
    private function validate(array $config, array $post, bool $isEdit): array
    {
        $data = [];
        $errors = [];

        foreach ($config['fields'] as $field => $meta) {
            if ($meta['type'] === 'readonly') {
                continue;
            }
            if ($field === 'password') {
                continue; // обрабатывается отдельно
            }

            $raw = $post[$field] ?? '';
            $value = is_string($raw) ? trim($raw) : $raw;

            if (!empty($meta['required']) && $value === '') {
                $errors[$field] = ($meta['label'] ?? $field) . ' — обязательное поле.';
                $data[$field] = $value;
                continue;
            }

            switch ($meta['type']) {
                case 'number':
                    if ($value !== '' && !preg_match('/^-?\d+$/', (string)$value)) {
                        $errors[$field] = ($meta['label'] ?? $field) . ' должно быть целым числом.';
                    }
                    break;
                case 'decimal':
                    if ($value !== '' && !preg_match('/^-?\d+(\.\d+)?$/', (string)$value)) {
                        $errors[$field] = ($meta['label'] ?? $field) . ' должно быть числом.';
                    }
                    break;
                case 'select':
                    if ($value !== '' && !array_key_exists($value, $meta['options'] ?? [])) {
                        $errors[$field] = 'Недопустимое значение поля ' . ($meta['label'] ?? $field) . '.';
                    }
                    break;
                case 'select_fk':
                    if ($value !== '' && !preg_match('/^\d+$/', (string)$value)) {
                        $errors[$field] = 'Недопустимый выбор для ' . ($meta['label'] ?? $field) . '.';
                    }
                    break;
                case 'text':
                case 'textarea':
                    if (is_string($value) && (function_exists('mb_strlen') ? mb_strlen($value) : strlen($value)) > 5000) {
                        $errors[$field] = ($meta['label'] ?? $field) . ' слишком длинное значение.';
                    }
                    break;
            }

            $data[$field] = $value;
        }

        return [$data, $errors];
    }
}
