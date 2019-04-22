import sqlite3
import datetime
song_db = "__HOME__/songs.db"

def handle_post(request):
	conn = sqlite3.connect(song_db)
	c = conn.cursor()
	c.execute('''CREATE TABLE IF NOT EXISTS song_table (song_name text,song text, timing timestamp);''')

	songname = request['form']['songName']
	song = request['form']['musicString']

	c.execute('''INSERT into song_table VALUES (?,?,?);''', (songname,song,datetime.datetime.now()))

	db_stuff = c.execute('''SELECT * FROM song_table''').fetchall()
	outs = ''
	for thing in db_stuff:
		outs += 'song name: ' + thing[0] + ', song: ' + thing[1] + ', timestamp: ' + thing[2] + '\n'
	conn.commit()
	conn.close()

	return outs

def request_handler(request):
	if request['method'] == 'POST':
		return handle_post(request)
	return request
