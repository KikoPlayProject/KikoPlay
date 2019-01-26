CREATE TABLE "task" (
"TaskID"  TEXT NOT NULL,
"Title"  TEXT,
"Dir"  TEXT,
"CTime"  INTEGER,
"FTime"  INTEGER,
"TLength"  INTEGER,
"CLength"  INTEGER,
"URI"  TEXT,
"SFIndexes"  TEXT,
"Torrent"  BLOB,
PRIMARY KEY ("TaskID")
);