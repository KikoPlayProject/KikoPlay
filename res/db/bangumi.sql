CREATE TABLE "anime" (
"Anime"  TEXT NOT NULL,
"AddTime"  INTEGER NOT NULL,
"EpCount"  INTEGER,
"AirDate"  TEXT(10),
"Desc"  TEXT,
"URL"  TEXT,
"ScriptId"  TEXT,
"ScriptData"  TEXT,
"Staff"  TEXT,
"CoverURL" TEXT,
"Cover"  BLOB,
PRIMARY KEY ("Anime" ASC)
);
CREATE INDEX "Anime_I"
ON "anime" ("Anime" ASC);

CREATE TABLE "character" (
"Anime"  TEXT NOT NULL,
"Name"  TEXT NOT NULL,
"Link"  TEXT,
"Actor"  TEXT,
"ImageURL"  TEXT,
"Image"  BLOB,
CONSTRAINT "Anime" FOREIGN KEY ("Anime") REFERENCES "anime" ("Anime") ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE INDEX "Anime_C"
ON "character" ("Anime" ASC);

CREATE TABLE "episode" (
"Anime"  TEXT NOT NULL,
"Name"  TEXT,
"Type"  INTEGER,
"EpIndex"  REAL,
"LastPlayTime"  INTEGER,
"FinishTime"  INTEGER,
"LocalFile"  TEXT NOT NULL,
CONSTRAINT "Anime" FOREIGN KEY ("Anime") REFERENCES "anime" ("Anime") ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE INDEX "Anime_E"
ON "episode" ("Anime" ASC);

CREATE TABLE "tag" (
"Anime"  TEXT NOT NULL,
"Tag"  TEXT NOT NULL,
PRIMARY KEY ("Anime", "Tag"),
CONSTRAINT "Anime" FOREIGN KEY ("Anime") REFERENCES "anime" ("Anime") ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE "alias" (
"Alias"  TEXT NOT NULL ON CONFLICT IGNORE,
"Anime"  TEXT,
PRIMARY KEY ("Alias" ASC)
);

CREATE INDEX "Alias_Index"
ON "alias" ("Alias" ASC);

CREATE TABLE "image" (
"Anime"  TEXT,
"TimeId"  INTEGER,
"Type"  INTEGER,
"Info"  TEXT,
"Thumb"  BLOB,
"Data"  BLOB,
PRIMARY KEY ("Anime","TimeId","Type"),
CONSTRAINT "Anime" FOREIGN KEY ("Anime") REFERENCES "anime" ("Anime") ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE INDEX "Time_Index"
ON "image" ("TimeId" ASC);
