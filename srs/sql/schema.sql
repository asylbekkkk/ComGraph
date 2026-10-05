-- =====================================================================
-- Diploma Topics App — schema.sql
-- Курс: Деректер қоры қосымшаларын құру (Database Application Development)
-- Задача 1 (СРС 1): структура БД + тестовые (синтетические) данные
-- Импорт: phpMyAdmin -> Import, либо:
--   mysql -u root -p < schema.sql
-- =====================================================================

CREATE DATABASE IF NOT EXISTS diploma_topics
    CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE diploma_topics;

SET FOREIGN_KEY_CHECKS = 0;

DROP TABLE IF EXISTS applications;
DROP TABLE IF EXISTS topics;
DROP TABLE IF EXISTS grades;
DROP TABLE IF EXISTS subjects;
DROP TABLE IF EXISTS students;
DROP TABLE IF EXISTS teachers;
DROP TABLE IF EXISTS users;

SET FOREIGN_KEY_CHECKS = 1;

-- ---------------------------------------------------------------------
-- Пользователи + роли (student / teacher / moderator / admin)
-- ---------------------------------------------------------------------
CREATE TABLE users (
    id            INT AUTO_INCREMENT PRIMARY KEY,
    login         VARCHAR(50)  UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role          ENUM('student','teacher','moderator','admin') NOT NULL,
    full_name     VARCHAR(150) NOT NULL,
    created_at    TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- ---------------------------------------------------------------------
-- Мұғалімдер (в реальности — заполняется парсингом ПСС ҚазҰУ)
-- ---------------------------------------------------------------------
CREATE TABLE teachers (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    user_id    INT NULL,
    full_name  VARCHAR(150) NOT NULL,
    department VARCHAR(150),
    position   VARCHAR(100),
    email      VARCHAR(100),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB;

-- ---------------------------------------------------------------------
-- Студенттер — базовая академ. инфа
-- ---------------------------------------------------------------------
CREATE TABLE students (
    id          INT AUTO_INCREMENT PRIMARY KEY,
    user_id     INT NOT NULL,
    faculty     VARCHAR(150),
    program     VARCHAR(150),
    study_group VARCHAR(50),
    course_year TINYINT,
    gpa         DECIMAL(3,2),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- ---------------------------------------------------------------------
-- Пәндер (справочник)
-- ---------------------------------------------------------------------
CREATE TABLE subjects (
    id      INT AUTO_INCREMENT PRIMARY KEY,
    code    VARCHAR(20),
    title   VARCHAR(200),
    credits TINYINT,
    ects    TINYINT
) ENGINE=InnoDB;

-- ---------------------------------------------------------------------
-- Оценки студента по пәндер
-- ---------------------------------------------------------------------
CREATE TABLE grades (
    id            INT AUTO_INCREMENT PRIMARY KEY,
    student_id    INT NOT NULL,
    subject_id    INT NOT NULL,
    semester      TINYINT,
    score_percent TINYINT,
    letter_grade  VARCHAR(3),
    gpa_point     DECIMAL(3,2),
    FOREIGN KEY (student_id) REFERENCES students(id) ON DELETE CASCADE,
    FOREIGN KEY (subject_id) REFERENCES subjects(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- ---------------------------------------------------------------------
-- Тақырыптар — учитель предлагает, модератор одобряет
-- ---------------------------------------------------------------------
CREATE TABLE topics (
    id          INT AUTO_INCREMENT PRIMARY KEY,
    title       VARCHAR(255) NOT NULL,
    teacher_id  INT NOT NULL,
    description TEXT,
    status      ENUM('pending','approved','rejected','taken') DEFAULT 'pending',
    reviewed_by INT NULL,
    reviewed_at TIMESTAMP NULL,
    FOREIGN KEY (teacher_id) REFERENCES teachers(id) ON DELETE CASCADE,
    FOREIGN KEY (reviewed_by) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB;

-- ---------------------------------------------------------------------
-- Заявки студента на тему
-- ---------------------------------------------------------------------
CREATE TABLE applications (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    student_id INT NOT NULL,
    topic_id   INT NOT NULL,
    status     ENUM('pending','approved','rejected') DEFAULT 'pending',
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (student_id) REFERENCES students(id) ON DELETE CASCADE,
    FOREIGN KEY (topic_id) REFERENCES topics(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- =====================================================================
-- SEED DATA (синтетика, без реальных ФИО/оценок)
-- Пароль для ВСЕХ пользователей ниже: "Password123!"
-- Хеш ниже сгенерирован через password_hash('Password123!', PASSWORD_BCRYPT)
-- =====================================================================
-- Один и тот же bcrypt-хеш используется для всех демо-пользователей.
SET @demo_hash = '$2y$10$Sc4iWQi8liouxZAdr0tsv.4pKNFrj96AYKnWmWoHeDkEQkKvUxiF6'; -- Password123!

INSERT INTO users (login, password_hash, role, full_name) VALUES
('admin',      @demo_hash, 'admin',     'Айдар Системный Администратор'),
('moderator1', @demo_hash, 'moderator', 'Гульнара Модераторова'),
('teacher1',   @demo_hash, 'teacher',   'Серік Дәулетов'),
('teacher2',   @demo_hash, 'teacher',   'Айгүл Нұрланова'),
('teacher3',   @demo_hash, 'teacher',   'Марат Қасымов'),
('student1',   @demo_hash, 'student',   'Данияр Ахметов'),
('student2',   @demo_hash, 'student',   'Асель Жандарбекова'),
('student3',   @demo_hash, 'student',   'Ержан Тоқтаров'),
('student4',   @demo_hash, 'student',   'Мадина Сәрсенова'),
('student5',   @demo_hash, 'student',   'Нұрсұлтан Ерболатов'),
('student6',   @demo_hash, 'student',   'Динара Қалиева');

INSERT INTO teachers (user_id, full_name, department, position, email) VALUES
((SELECT id FROM users WHERE login='teacher1'), 'Серік Дәулетов',  'Информатика және ДҚ кафедрасы', 'Доцент', 'daulet.s@kaznu.kz'),
((SELECT id FROM users WHERE login='teacher2'), 'Айгүл Нұрланова', 'Информатика және ДҚ кафедрасы', 'Профессор', 'nurlanova.a@kaznu.kz'),
((SELECT id FROM users WHERE login='teacher3'), 'Марат Қасымов',   'Ақпараттық жүйелер кафедрасы',  'Аға оқытушы', 'kassymov.m@kaznu.kz'),
(NULL, 'Ботагоз Ищанова',  'Ақпараттық жүйелер кафедрасы',  'Доцент', 'ishanova.b@kaznu.kz'),
(NULL, 'Тимур Байжанов',   'Информатика және ДҚ кафедрасы', 'Аға оқытушы', 'baizhanov.t@kaznu.kz');

INSERT INTO students (user_id, faculty, program, study_group, course_year, gpa) VALUES
((SELECT id FROM users WHERE login='student1'), 'Механика-математика факультеті', '6B06104 Компьютерлік ғылымдар', 'CS-21-1', 3, 3.45),
((SELECT id FROM users WHERE login='student2'), 'Механика-математика факультеті', '6B06104 Компьютерлік ғылымдар', 'CS-21-1', 3, 3.82),
((SELECT id FROM users WHERE login='student3'), 'Механика-математика факультеті', '6B06104 Компьютерлік ғылымдар', 'CS-21-2', 3, 2.95),
((SELECT id FROM users WHERE login='student4'), 'Механика-математика факультеті', '6B06104 Компьютерлік ғылымдар', 'CS-22-1', 2, 3.60),
((SELECT id FROM users WHERE login='student5'), 'Механика-математика факультеті', '6B06104 Компьютерлік ғылымдар', 'CS-22-1', 2, 3.10),
((SELECT id FROM users WHERE login='student6'), 'Механика-математика факультеті', '6B06104 Компьютерлік ғылымдар', 'CS-21-2', 3, 3.90);

INSERT INTO subjects (code, title, credits, ects) VALUES
('CSE2201', 'Деректер қорын құру', 3, 5),
('CSE2202', 'Алгоритмдер мен деректер құрылымы', 3, 5),
('CSE3301', 'Веб-технологиялар', 3, 5),
('CSE3302', 'Операциялық жүйелер', 3, 5),
('CSE3303', 'Компьютерлік желілер', 3, 5),
('CSE2203', 'Объектіге бағытталған бағдарламалау', 3, 5),
('CSE4401', 'Ақпараттық қауіпсіздік негіздері', 2, 4),
('CSE4402', 'Бағдарламалық жобалау', 3, 5);

INSERT INTO grades (student_id, subject_id, semester, score_percent, letter_grade, gpa_point) VALUES
(1,1,3,88,'A-',3.67),(1,2,3,75,'B',3.00),(1,3,4,92,'A',4.00),(1,4,4,68,'C+',2.33),
(2,1,3,95,'A',4.00),(2,2,3,90,'A-',3.67),(2,3,4,85,'B+',3.33),(2,6,2,93,'A',4.00),
(3,1,3,60,'C',2.00),(3,2,3,72,'B-',2.67),(3,3,4,58,'C-',1.67),(3,6,2,80,'B',3.00),
(4,1,1,85,'B+',3.33),(4,2,2,90,'A-',3.67),(4,6,1,88,'A-',3.67),
(5,1,1,70,'B-',2.67),(5,2,2,65,'C+',2.33),(5,6,1,78,'B',3.00),
(6,1,3,97,'A',4.00),(6,3,4,94,'A',4.00),(6,4,4,89,'A-',3.67),(6,7,4,91,'A',4.00);

INSERT INTO topics (title, teacher_id, description, status, reviewed_by, reviewed_at) VALUES
('Веб-қосымша: дипломдық жұмыс тақырыптарын таңдау жүйесі', 1, 'PHP + MySQL негізінде CRUD әкімші панелі мен студент интерфейсі.', 'approved', 2, NOW()),
('Мобильді қосымша: студенттердің GPA трекері', 1, 'Android/Flutter қосымшасы арқылы GPA есептеу және көрсету.', 'pending', NULL, NULL),
('Машиналық оқыту негізінде студенттердің үлгерімін болжау', 2, 'Python, scikit-learn көмегімен үлгерімді болжау моделі.', 'approved', 2, NOW()),
('Чат-бот: оқу үрдісі бойынша ақпараттық сервис', 2, 'NLP негізінде студенттерге ақпарат беретін telegram-бот.', 'taken', 2, NOW()),
('Деректер қорын оңтайландыру: индекстеу стратегиялары', 3, 'Үлкен көлемдегі деректер қорында индекстеудің әсерін зерттеу.', 'pending', NULL, NULL),
('Микросервистік архитектура негізінде оқу порталы', 3, 'Docker + Kubernetes арқылы микросервистерді жобалау.', 'approved', 2, NOW()),
('Компьютерлік көру: оқу залындағы бос орындарды анықтау', 1, 'OpenCV негізінде камера арқылы бос орындарды тану.', 'rejected', 2, NOW()),
('Блокчейн негізінде дипломдарды растау жүйесі', 2, 'Ethereum смарт-келісімшарттары арқылы дипломды растау.', 'pending', NULL, NULL),
('Ұсыныстар жүйесі: студенттерге пән таңдауға көмек', 3, 'Collaborative filtering негізінде пән ұсыну алгоритмі.', 'approved', 2, NOW()),
('IoT негізінде кампус мониторингі', 1, 'Датчиктер арқылы кампус ресурстарын бақылау жүйесі.', 'pending', NULL, NULL),
('Univer жүйесімен интеграцияланған тесттеу платформасы', 2, 'REST API арқылы сыртқы жүйелермен интеграция.', 'taken', 2, NOW()),
('Табиғи тілді өңдеу негізінде плагиатты анықтау жүйесі', 3, 'Дипломдық жұмыстардағы плагиатты анықтау алгоритмі.', 'approved', 2, NOW());

INSERT INTO applications (student_id, topic_id, status, applied_at) VALUES
(1, 1, 'approved', NOW()),
(2, 3, 'pending',  NOW()),
(3, 6, 'pending',  NOW()),
(4, 4, 'approved', NOW()),
(5, 9, 'rejected', NOW()),
(6, 1, 'pending',  NOW()),
(2, 9, 'pending',  NOW()),
(6, 11,'approved', NOW());
