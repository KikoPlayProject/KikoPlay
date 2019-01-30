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
PRIMARY KEY ("TaskID")
);
CREATE INDEX "TaskID"
ON "task" ("TaskID" ASC);

CREATE TABLE "torrent" (
"TaskID"  TEXT NOT NULL,
"Torrent"  BLOB,
CONSTRAINT "TaskID" FOREIGN KEY ("TaskID") REFERENCES "task" ("TaskID") ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE INDEX "TaskID_T"
ON "torrent" ("TaskID" ASC);
