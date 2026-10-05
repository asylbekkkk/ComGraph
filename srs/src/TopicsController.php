<?php
declare(strict_types=1);

/**
 * TopicsController — отдельная страница модерации тем: approve/reject.
 * Доступна ролям moderator и admin (server-side проверка).
 */
class TopicsController
{
    public function moderate(): void
    {
        Auth::requireRole(['moderator', 'admin']);
        $pdo = Database::connection();

        $stmt = $pdo->prepare(
            "SELECT t.id, t.title, t.description, t.status, tc.full_name AS teacher_name
             FROM topics t
             JOIN teachers tc ON tc.id = t.teacher_id
             WHERE t.status = 'pending'
             ORDER BY t.id DESC"
        );
        $stmt->execute();
        $pendingTopics = $stmt->fetchAll();

        require BASE_PATH . '/views/topics/moderate.php';
    }

    public function decide(): void
    {
        Auth::requireRole(['moderator', 'admin']);
        Csrf::verifyOrFail();

        $id = (int)($_POST['id'] ?? 0);
        $decision = (string)($_POST['decision'] ?? '');

        if ($id <= 0 || !in_array($decision, ['approved', 'rejected'], true)) {
            http_response_code(400);
            die('Некорректный запрос.');
        }

        $pdo = Database::connection();
        // обновляем только записи, которые реально в статусе pending
        $stmt = $pdo->prepare(
            "UPDATE topics SET status = :status, reviewed_by = :uid, reviewed_at = NOW()
             WHERE id = :id AND status = 'pending'"
        );
        $stmt->execute([
            'status' => $decision,
            'uid' => Auth::user()['id'],
            'id' => $id,
        ]);

        header('Location: index.php?r=topics/moderate');
        exit;
    }
}
