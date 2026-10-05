# Дипломдық тақырыптар — Задача 1 (СРС 1)

Курс: **Деректер қоры қосымшаларын құру** (Database Application Development)
ОП 6B06104 «Компьютерлік ғылымдар», осенний семестр 2026.

Первая (PHP) версия: БД + админ-панель с полным CRUD по всем таблицам
+ синтетические тестовые данные. Стек: чистый PHP + PDO, MySQL/MariaDB
(XAMPP), собственный лёгкий MVC (без фреймворка).

## Структура проекта

```
diploma-topics-app/
├── config/
│   ├── config.php        # DB-настройки, сессия, автозагрузка классов
│   └── entities.php       # конфиг сущностей для универсального CRUD
├── sql/
│   └── schema.sql          # DDL + синтетические тестовые данные
├── src/                     # "Model" + контроллеры (Core/Controllers)
│   ├── Database.php         # PDO singleton
│   ├── Auth.php             # логин/логаут, server-side проверка ролей
│   ├── Csrf.php             # CSRF-токены
│   ├── CrudModel.php        # универсальная модель CRUD (PDO prepared)
│   ├── AuthController.php
│   ├── DashboardController.php
│   ├── CrudController.php   # CRUD для users/teachers/students/subjects/grades/topics/applications
│   └── TopicsController.php # модерация тем (approve/reject)
├── views/                   # "View" — шаблоны (htmlspecialchars везде)
│   ├── layout/, auth/, dashboard/, crud/, topics/, errors/
├── public/
│   ├── index.php            # единственная точка входа (front controller)
│   └── assets/css/style.css
└── pentest/
    └── REPORT_TEMPLATE.md   # шаблон отчёта по самостоятельному пентесту
```

## Установка (XAMPP)

1. Скопируйте папку `diploma-topics-app` в `htdocs` (например
   `C:\xampp\htdocs\diploma-topics-app` или `/opt/lampp/htdocs/...`).
2. Запустите Apache и MySQL в панели XAMPP.
3. Откройте **phpMyAdmin** → вкладка *Импорт* → выберите файл
   `sql/schema.sql` → Импортировать. Это создаст базу `diploma_topics`
   со всеми таблицами и тестовыми данными.
4. При необходимости отредактируйте `config/config.php`, если у вас
   отличаются `DB_HOST` / `DB_USER` / `DB_PASS` (по умолчанию — типичные
   значения для XAMPP: `localhost`, `root`, пустой пароль).
5. Откройте в браузере:
   `http://localhost/diploma-topics-app/public/index.php`

## Демо-доступы

Все пользователи ниже имеют пароль **`Password123!`**

| Логин       | Роль      | Возможности                                  |
|-------------|-----------|-----------------------------------------------|
| `admin`     | admin     | полный CRUD по всем 7 таблицам                |
| `moderator1`| moderator | страница модерации тем (approve/reject)       |
| `teacher1`  | teacher   | вход в систему (публичный кабинет — этап 2)   |
| `student1`  | student   | вход в систему (публичный кабинет — этап 2)   |

## Что реализовано (Задача 1)

- [x] Схема БД точно по ТЗ (`users, teachers, students, subjects, grades, topics, applications`) с внешними ключами.
- [x] Логин через `password_hash`/`password_verify`, сессии с `httponly`-cookie, `session_regenerate_id` при входе.
- [x] Server-side проверка роли на каждой защищённой странице (`Auth::requireRole`), а не только скрытие ссылок в UI.
- [x] CRUD-страницы для всех 7 таблиц: список с пагинацией, форма добавления, форма редактирования, удаление с подтверждением (JS `confirm` + POST + CSRF).
- [x] Отдельное действие «одобрить/отклонить» для тем — доступно роли `moderator` (и `admin`), с записью `reviewed_by`/`reviewed_at`.
- [x] Серверная валидация всех форм (обязательные поля, типы: число/decimal/enum/внешний ключ) — независимо от клиентской.
- [x] Все запросы к БД — только через PDO prepared statements (`PDO::ATTR_EMULATE_PREPARES = false`), без конкатенации значений в SQL.
- [x] Пароли — только bcrypt (`password_hash`).
- [x] CSRF-токен в каждой форме (`Csrf::field()` / `Csrf::verifyOrFail()`).
- [x] `htmlspecialchars` на весь вывод пользовательских данных.
- [x] Синтетические тестовые данные: 5 мұғалімдер, 12 тем (pending/approved/rejected/taken), 6 студентов с оценками, 8 заявок.

## Что не входит в Задачу 1 (следующий этап)

- Публичные страницы для студентов/мұғалімдер (список тем с фильтрами, карточка мұғалім, профиль, студент историясы).
- Реальный парсинг данных с ПСС ҚазҰУ.
- Переход на Django/PostgreSQL (2-я СРС).

## Архитектурные заметки

CRUD реализован как **один универсальный контроллер** (`CrudController`)
и **одна универсальная модель** (`CrudModel`), которые управляются
конфигом `config/entities.php` (таблица, колонки, типы полей, внешние
ключи). Это устраняет дублирование кода между 7 почти одинаковыми
CRUD-модулями, но при этом каждая сущность полностью настраиваема
(свои обязательные поля, свои select/FK-списки, спец-логика для
`users.password` и `topics.reviewed_at`).
