insert into "group"(name, password)
values ('main', '1234'),
       ('new life', '1234'),
       ('lions', '1234');

select password
from "Messenger".public.group
where name = 'main';

select m.sender_name, m.message, m.time
from message m
         inner join "group" g on g.id = m.group_id
where g.name = 'main';

insert into message (group_id, sender_name, message, time)
values (1, 'ildar', 'hello world', '22:30');