<?php
/**
 * config/entities.php
 * Конфигурация сущностей для универсальной (config-driven) CRUD-панели.
 * Каждая запись описывает: таблицу, PK, заголовок, колонки формы/списка,
 * типы полей и внешние ключи (для выпадающих списков).
 *
 * Типы полей:
 *  text | textarea | number | decimal | select | select_fk | password | readonly
 */

function entities_config(): array
{
    return [
        'users' => [
            'table' => 'users',
            'pk' => 'id',
            'label' => 'Пользователи',
            'roles' => ['admin'],
            'system_columns' => ['password_hash'], // формадан емес, контроллер қоятын бағандар
            'list_columns' => ['id', 'login', 'role', 'full_name', 'created_at'],
            'fields' => [
                'id'         => ['label' => 'ID', 'type' => 'readonly'],
                'login'      => ['label' => 'Логин', 'type' => 'text', 'required' => true],
                'password'   => ['label' => 'Пароль', 'type' => 'password', 'required' => false,
                                  'help' => 'Оставьте пустым, чтобы не менять пароль при редактировании'],
                'role'       => ['label' => 'Роль', 'type' => 'select', 'required' => true,
                                  'options' => ['student' => 'student', 'teacher' => 'teacher',
                                                'moderator' => 'moderator', 'admin' => 'admin']],
                'full_name'  => ['label' => 'ФИО', 'type' => 'text', 'required' => true],
                'created_at' => ['label' => 'Создан', 'type' => 'readonly'],
            ],
        ],

        'teachers' => [
            'table' => 'teachers',
            'pk' => 'id',
            'label' => 'Мұғалімдер',
            'roles' => ['admin'],
            'list_columns' => ['id', 'full_name', 'department', 'position', 'email'],
            'fields' => [
                'id'         => ['label' => 'ID', 'type' => 'readonly'],
                'user_id'    => ['label' => 'Связанный пользователь', 'type' => 'select_fk', 'required' => false,
                                  'fk_table' => 'users', 'fk_pk' => 'id', 'fk_label' => 'login',
                                  'fk_extra_where' => "role='teacher'"],
                'full_name'  => ['label' => 'ФИО', 'type' => 'text', 'required' => true],
                'department' => ['label' => 'Кафедра', 'type' => 'text', 'required' => false],
                'position'   => ['label' => 'Должность', 'type' => 'text', 'required' => false],
                'email'      => ['label' => 'Email', 'type' => 'text', 'required' => false],
            ],
        ],

        'students' => [
            'table' => 'students',
            'pk' => 'id',
            'label' => 'Студенттер',
            'roles' => ['admin'],
            'list_columns' => ['id', 'user_id', 'faculty', 'study_group', 'course_year', 'gpa'],
            'fields' => [
                'id'          => ['label' => 'ID', 'type' => 'readonly'],
                'user_id'     => ['label' => 'Пользователь', 'type' => 'select_fk', 'required' => true,
                                   'fk_table' => 'users', 'fk_pk' => 'id', 'fk_label' => 'login',
                                   'fk_extra_where' => "role='student'"],
                'faculty'     => ['label' => 'Факультет', 'type' => 'text', 'required' => false],
                'program'     => ['label' => 'Мамандық', 'type' => 'text', 'required' => false],
                'study_group' => ['label' => 'Топ', 'type' => 'text', 'required' => false],
                'course_year' => ['label' => 'Курс', 'type' => 'number', 'required' => false],
                'gpa'         => ['label' => 'GPA', 'type' => 'decimal', 'required' => false],
            ],
        ],

        'subjects' => [
            'table' => 'subjects',
            'pk' => 'id',
            'label' => 'Пәндер',
            'roles' => ['admin'],
            'list_columns' => ['id', 'code', 'title', 'credits', 'ects'],
            'fields' => [
                'id'      => ['label' => 'ID', 'type' => 'readonly'],
                'code'    => ['label' => 'Код', 'type' => 'text', 'required' => false],
                'title'   => ['label' => 'Атауы', 'type' => 'text', 'required' => true],
                'credits' => ['label' => 'Кредит', 'type' => 'number', 'required' => false],
                'ects'    => ['label' => 'ECTS', 'type' => 'number', 'required' => false],
            ],
        ],

        'grades' => [
            'table' => 'grades',
            'pk' => 'id',
            'label' => 'Бағалар',
            'roles' => ['admin'],
            'list_columns' => ['id', 'student_id', 'subject_id', 'semester', 'score_percent', 'letter_grade', 'gpa_point'],
            'fields' => [
                'id'            => ['label' => 'ID', 'type' => 'readonly'],
                'student_id'    => ['label' => 'Студент', 'type' => 'select_fk', 'required' => true,
                                     'fk_table' => 'students', 'fk_pk' => 'id', 'fk_label' => 'faculty',
                                     'fk_label_join' => ['users', 'id', 'user_id', 'full_name']],
                'subject_id'    => ['label' => 'Пән', 'type' => 'select_fk', 'required' => true,
                                     'fk_table' => 'subjects', 'fk_pk' => 'id', 'fk_label' => 'title'],
                'semester'      => ['label' => 'Семестр', 'type' => 'number', 'required' => false],
                'score_percent' => ['label' => 'Балл (%)', 'type' => 'number', 'required' => false],
                'letter_grade'  => ['label' => 'Әріптік баға', 'type' => 'text', 'required' => false],
                'gpa_point'     => ['label' => 'GPA балл', 'type' => 'decimal', 'required' => false],
            ],
        ],

        'topics' => [
            'table' => 'topics',
            'pk' => 'id',
            'label' => 'Тақырыптар',
            'roles' => ['admin'],
            'system_columns' => ['reviewed_at'],
            'list_columns' => ['id', 'title', 'teacher_id', 'status', 'reviewed_by'],
            'fields' => [
                'id'          => ['label' => 'ID', 'type' => 'readonly'],
                'title'       => ['label' => 'Атауы', 'type' => 'text', 'required' => true],
                'teacher_id'  => ['label' => 'Мұғалім', 'type' => 'select_fk', 'required' => true,
                                    'fk_table' => 'teachers', 'fk_pk' => 'id', 'fk_label' => 'full_name'],
                'description' => ['label' => 'Сипаттама', 'type' => 'textarea', 'required' => false],
                'status'      => ['label' => 'Статус', 'type' => 'select', 'required' => true,
                                    'options' => ['pending' => 'pending', 'approved' => 'approved',
                                                  'rejected' => 'rejected', 'taken' => 'taken']],
                'reviewed_by' => ['label' => 'Проверил (user)', 'type' => 'select_fk', 'required' => false,
                                    'fk_table' => 'users', 'fk_pk' => 'id', 'fk_label' => 'login',
                                    'fk_extra_where' => "role IN ('moderator','admin')"],
                'reviewed_at' => ['label' => 'Дата проверки', 'type' => 'readonly'],
            ],
        ],

        'applications' => [
            'table' => 'applications',
            'pk' => 'id',
            'label' => 'Заявки',
            'roles' => ['admin'],
            'list_columns' => ['id', 'student_id', 'topic_id', 'status', 'applied_at'],
            'fields' => [
                'id'         => ['label' => 'ID', 'type' => 'readonly'],
                'student_id' => ['label' => 'Студент', 'type' => 'select_fk', 'required' => true,
                                   'fk_table' => 'students', 'fk_pk' => 'id', 'fk_label' => 'faculty',
                                   'fk_label_join' => ['users', 'id', 'user_id', 'full_name']],
                'topic_id'   => ['label' => 'Тақырып', 'type' => 'select_fk', 'required' => true,
                                   'fk_table' => 'topics', 'fk_pk' => 'id', 'fk_label' => 'title'],
                'status'     => ['label' => 'Статус', 'type' => 'select', 'required' => true,
                                   'options' => ['pending' => 'pending', 'approved' => 'approved', 'rejected' => 'rejected']],
                'applied_at' => ['label' => 'Дата подачи', 'type' => 'readonly'],
            ],
        ],
    ];
}
