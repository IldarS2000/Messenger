create table "user"
(
    id       serial primary key,
    name     varchar(32) unique not null,
    password varchar(128)       not null
);

create table "group"
(
    id       serial primary key,
    name     varchar(32) unique not null,
    password varchar(128)       not null
);

create table "message"
(
    id          serial primary key,
    group_name  varchar(32)   not null, -- temp solutions
    --group_id int           not null,
    sender_name varchar(32)   not null, -- temp solutions
    --user_id  int           not null,
    message     varchar(2048) not null,
    time        varchar(6)    not null
    -- time        timestamp     not null,

    --constraint message_group_fk foreign key (group_id) references "group" (id)
    --constraint message_user_fk foreign key (user_id) references "user" (id)
);

create table "group_user"
(
    user_id  int not null,
    group_id int not null,

    constraint group_user_pk primary key (user_id, group_id),
    constraint user_fk foreign key (user_id) references "user" (id),
    constraint group_fk foreign key (group_id) references "group" (id)
);

