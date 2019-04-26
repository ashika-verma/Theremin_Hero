import sqlite3
import datetime
song_db = "__HOME__/songs.db"


def handle_post(request):
	conn = sqlite3.connect(song_db)
	c = conn.cursor()
	c.execute('''CREATE TABLE IF NOT EXISTS song_table (id INTEGER PRIMARY KEY AUTOINCREMENT, 
														songName TEXT, 
														song TEXT, 
														timing TIMESTAMP);''')

	songName = request['form']['songName']
	song = request['form']['musicString']

	c.execute('''INSERT OR IGNORE into song_table VALUES (NULL, ?,?,?);''', (songName,song,datetime.datetime.now()))

	dbSongs = c.execute('''SELECT * FROM song_table''').fetchall()
	outs = ''
	for thing in dbSongs:
		outs += 'song id: ' + str(thing[0]) + ', song name: ' + thing[1] + ', song: ' + thing[2] + ', timestamp: ' + thing[3] + '\n'

	conn.commit()
	conn.close()

	return outs


def handle_get(request):
	conn = sqlite3.connect(song_db)
	c = conn.cursor()
	c.execute('''CREATE TABLE IF NOT EXISTS song_table (id INTEGER PRIMARY KEY AUTOINCREMENT, 
														songName TEXT UNIQUE, 
														song TEXT, 
														timing TIMESTAMP);''')

	if "songName" in request["args"]:
		dbSongs = c.execute('''SELECT * FROM song_table WHERE songName = ? ORDER BY timing''', (songName)).fetchall()
	else:
		dbSongs = c.execute('''SELECT id,songName FROM song_table ORDER BY timing ASC''').fetchall()
	songs = ''
	for song in dbSongs:
		songs += song[0] + ',' + song[1] + '\n'

	conn.commit()
	conn.close()

	return songs


def request_handler(request):
	if request['method'] == 'POST':
		return handle_post(request)
	else:
		return handle_get(request)
