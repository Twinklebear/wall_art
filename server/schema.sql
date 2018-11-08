drop table if exists images;
create table images (
    id integer primary key autoincrement,
    title text not null,
    artist text not null,
    work_type text not null,
    has_nudity boolean not null,
    filename text not null
);

