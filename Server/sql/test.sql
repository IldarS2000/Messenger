insert into "group"(name, password)
values ('main', '1234'),
       ('new life', '1234'),
       ('lions', '1234');

SELECT password
from "group" g
where g.name = 'main';