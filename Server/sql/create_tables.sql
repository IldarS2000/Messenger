create table "Messenger".public.user
(
    id       serial primary key,
    name     varchar(32) unique not null,
    password varchar(128)       not null
);

create table "Messenger".public.group
(
    id       serial primary key,
    name     varchar(32) unique not null,
    password varchar(128)       not null
);

create table "Messenger".public.message
(
    group_id    int           not null,
    sender_name varchar(32)   not null,
    message     varchar(2048) not null,
    time        timestamp     not null
);