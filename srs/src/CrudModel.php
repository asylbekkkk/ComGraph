<?php
declare(strict_types=1);

/**
 * CrudModel — универсальная модель для CRUD по любой таблице,
 * описанной в config/entities.php. Все запросы — через PDO prepared
 * statements (никакой конкатенации значений в SQL).
 */
class CrudModel
{
    private PDO $pdo;
    private array $config;
    private string $table;
    private string $pk;

    public function __construct(array $entityConfig)
    {
        $this->pdo = Database::connection();
        $this->config = $entityConfig;
        $this->table = $entityConfig['table'];
        $this->pk = $entityConfig['pk'];
    }

    /** Имя таблицы (проверено против конфига, безопасно для вставки в SQL как идентификатор) */
    private function tableIdent(): string
    {
        return '`' . preg_replace('/[^a-zA-Z0-9_]/', '', $this->table) . '`';
    }

    private function pkIdent(): string
    {
        return '`' . preg_replace('/[^a-zA-Z0-9_]/', '', $this->pk) . '`';
    }

    public function count(): int
    {
        $stmt = $this->pdo->query('SELECT COUNT(*) FROM ' . $this->tableIdent());
        return (int)$stmt->fetchColumn();
    }

    /** Постраничный список записей */
    public function paginate(int $page, int $perPage = 10): array
    {
        $page = max(1, $page);
        $offset = ($page - 1) * $perPage;
        $sql = 'SELECT * FROM ' . $this->tableIdent() . ' ORDER BY ' . $this->pkIdent() . ' DESC LIMIT :limit OFFSET :offset';
        $stmt = $this->pdo->prepare($sql);
        $stmt->bindValue(':limit', $perPage, PDO::PARAM_INT);
        $stmt->bindValue(':offset', $offset, PDO::PARAM_INT);
        $stmt->execute();
        return $stmt->fetchAll();
    }

    public function find(int $id): ?array
    {
        $stmt = $this->pdo->prepare('SELECT * FROM ' . $this->tableIdent() . ' WHERE ' . $this->pkIdent() . ' = :id LIMIT 1');
        $stmt->execute(['id' => $id]);
        $row = $stmt->fetch();
        return $row ?: null;
    }

    /**
     * Создать запись. $data — ассоциативный массив колонка => значение,
     * ключи фильтруются по описанным в конфиге полям (кроме readonly).
     */
    public function create(array $data): int
    {
        [$cols, $placeholders, $params] = $this->prepareWritable($data);
        $sql = 'INSERT INTO ' . $this->tableIdent() . ' (' . implode(', ', $cols) . ') VALUES (' . implode(', ', $placeholders) . ')';
        $stmt = $this->pdo->prepare($sql);
        $stmt->execute($params);
        return (int)$this->pdo->lastInsertId();
    }

    public function update(int $id, array $data): bool
    {
        [$cols, , $params] = $this->prepareWritable($data);
        if (empty($cols)) {
            return true;
        }
        $sets = array_map(fn($c) => $c . ' = :' . trim($c, '`'), $cols);
        $sql = 'UPDATE ' . $this->tableIdent() . ' SET ' . implode(', ', $sets) . ' WHERE ' . $this->pkIdent() . ' = :__id';
        $params['__id'] = $id;
        $stmt = $this->pdo->prepare($sql);
        return $stmt->execute($params);
    }

    public function delete(int $id): bool
    {
        $stmt = $this->pdo->prepare('DELETE FROM ' . $this->tableIdent() . ' WHERE ' . $this->pkIdent() . ' = :id');
        return $stmt->execute(['id' => $id]);
    }

    /** Отфильтровать входные данные строго по описанным колонкам (белый список) */
    private function prepareWritable(array $data): array
    {
        $cols = [];
        $placeholders = [];
        $params = [];
        foreach ($this->config['fields'] as $field => $meta) {
            if (in_array($meta['type'], ['readonly'], true)) {
                continue; // readonly-поля никогда не пишем из формы
            }
            if ($field === 'password') {
                continue; // пароль обрабатывается отдельно (хеширование) в контроллере
            }
            if (!array_key_exists($field, $data)) {
                continue;
            }
            $colIdent = '`' . preg_replace('/[^a-zA-Z0-9_]/', '', $field) . '`';
            $cols[] = $colIdent;
            $placeholders[] = ':' . $field;
            $value = $data[$field];
            $params[$field] = ($value === '') ? null : $value;
        }
        // Жүйелік бағандар (формадан емес, контроллер есептейді): password_hash, reviewed_at
        foreach (($this->config['system_columns'] ?? []) as $sys) {
            if (array_key_exists($sys, $data)) {
                $cols[] = '`' . preg_replace('/[^a-zA-Z0-9_]/', '', $sys) . '`';
                $placeholders[] = ':' . $sys;
                $params[$sys] = $data[$sys];
            }
        }
        return [$cols, $placeholders, $params];
    }

    /**
     * Опции для select_fk: [value => label]
     */
    public function fkOptions(array $fieldMeta): array
    {
        $table = '`' . preg_replace('/[^a-zA-Z0-9_]/', '', $fieldMeta['fk_table']) . '`';
        $pk = preg_replace('/[^a-zA-Z0-9_]/', '', $fieldMeta['fk_pk']);
        $labelCol = preg_replace('/[^a-zA-Z0-9_]/', '', $fieldMeta['fk_label']);

        if (!empty($fieldMeta['fk_label_join'])) {
            // [joinTable, joinPk, localFkCol, joinLabelCol] — для составных подписей (напр. students -> users.full_name)
            [$joinTable, $joinPk, $localFkCol, $joinLabelCol] = $fieldMeta['fk_label_join'];
            $joinTable = preg_replace('/[^a-zA-Z0-9_]/', '', $joinTable);
            $joinPk = preg_replace('/[^a-zA-Z0-9_]/', '', $joinPk);
            $localFkCol = preg_replace('/[^a-zA-Z0-9_]/', '', $localFkCol);
            $joinLabelCol = preg_replace('/[^a-zA-Z0-9_]/', '', $joinLabelCol);
            $sql = "SELECT t.$pk AS id, CONCAT(t.$labelCol, ' — ', j.$joinLabelCol) AS label
                    FROM $table t LEFT JOIN `$joinTable` j ON j.$joinPk = t.$localFkCol
                    ORDER BY t.$pk DESC";
            $stmt = $this->pdo->query($sql);
        } else {
            $where = '';
            if (!empty($fieldMeta['fk_extra_where'])) {
                // fk_extra_where задаётся только в доверенном конфиге (не из пользовательского ввода)
                $where = ' WHERE ' . $fieldMeta['fk_extra_where'];
            }
            $sql = "SELECT $pk AS id, $labelCol AS label FROM $table$where ORDER BY $pk DESC";
            $stmt = $this->pdo->query($sql);
        }

        $options = [];
        foreach ($stmt->fetchAll() as $row) {
            $options[$row['id']] = $row['label'] ?? ('#' . $row['id']);
        }
        return $options;
    }
}
