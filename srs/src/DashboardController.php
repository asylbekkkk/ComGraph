<?php
declare(strict_types=1);

class DashboardController
{
    public function index(): void
    {
        Auth::requireLogin();
        $user = Auth::user();
        $entities = entities_config();
        require BASE_PATH . '/views/dashboard/index.php';
    }
}
