import sqlite3
import datetime
song_db = "__HOME__/songs.db"


def handle_post(request):
	conn = sqlite3.connect(song_db)
	c = conn.cursor()
	c.execute('''CREATE TABLE IF NOT EXISTS songs (id INTEGER PRIMARY KEY AUTOINCREMENT, 
														songName TEXT, 
														song TEXT, 
														timing TIMESTAMP);''')

	songName = request['form']['songName']
	song = request['form']['musicString']

	c.execute('''INSERT OR IGNORE into songs VALUES (NULL,?,?,?);''', (songName,song,datetime.datetime.now()))

	dbSongs = c.execute('''SELECT * FROM songs''').fetchall()
	outs = ''
	for song in dbSongs:
		outs += 'song id: ' + str(song[0]) + ', song name: ' + song[1] + ', song: ' + song[2] + ', timestamp: ' + song[3] + '\n'

	conn.commit()
	conn.close()

	return outs


def handle_get(request):
	conn = sqlite3.connect(song_db)
	c = conn.cursor()
	c.execute('''CREATE TABLE IF NOT EXISTS songs (id INTEGER PRIMARY KEY AUTOINCREMENT, 
														songName TEXT UNIQUE, 
														song TEXT, 
														timing TIMESTAMP);''')

	if "songName" in request["args"]:
		songName = request["values"]["songName"]
		dbSongs = c.execute('''SELECT id,song FROM songs WHERE songName = ? ORDER BY timing ASC;''', (songName,)).fetchall()
	else:
		dbSongs = c.execute('''SELECT id,song FROM songs ORDER BY timing ASC;''').fetchall()
	songs = ''
	for song in dbSongs:
		songs += str(song[0]) + ',' + song[1] + '\n'

	conn.commit()
	conn.close()

	return songs


def request_handler(request):
	if request['method'] == 'POST':
		return handle_post(request)
	else:
		return handle_get(request)
