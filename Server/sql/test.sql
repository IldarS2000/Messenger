insert into "group"(name, password)
values ('main', '1234'),
       ('new life', '1234'),
       ('lions', '1234');

select password
from "Messenger".public.group
where name = 'main';