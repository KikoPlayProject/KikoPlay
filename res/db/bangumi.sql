CREATE TABLE "anime" (
"Anime"  TEXT NOT NULL,
"AddTime"  INTEGER NOT NULL,
"EpCount"  INTEGER,
"Summary"  TEXT,
"Date"  TEXT,
"Staff"  TEXT,
"BangumiID"  INTEGER,
"Cover"  BLOB,
PRIMARY KEY ("Anime" ASC)
);
CREATE INDEX "Anime"
ON "anime" ("Anime" ASC);

CREATE TABLE "character" (
"Anime"  TEXT NOT NULL,
"Name"  TEXT NOT NULL,
"NameCN"  TEXT NOT NULL,
"Actor"  TEXT,
"BangumiID"  INTEGER,
"Image"  BLOB,
CONSTRAINT "Anime" FOREIGN KEY ("Anime") REFERENCES "anime" ("Anime") ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE INDEX "Anime_C"
ON "character" ("Anime" ASC);

CREATE TABLE "eps" (
"Anime"  TEXT NOT NULL,
"Name"  TEXT,
"LocalFile"  TEXT,
"LastPlayTime"  TEXT,
CONSTRAINT "Anime" FOREIGN KEY ("Anime") REFERENCES "anime" ("Anime") ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE INDEX "Anime_E"
ON "eps" ("Anime" ASC);

CREATE TABLE "tag" (
"Anime"  TEXT NOT NULL,
"Tag"  TEXT NOT NULL,
PRIMARY KEY ("Anime", "Tag"),
CONSTRAINT "Anime" FOREIGN KEY ("Anime") REFERENCES "anime" ("Anime") ON DELETE CASCADE ON UPDATE CASCADE
);