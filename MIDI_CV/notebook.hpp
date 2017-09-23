enum NB_Mode { highest, lowest, velocity };

class Note {
        public:
                Note(char _pitch, char _velocity) :
                        pitch(_pitch), velocity(_velocity) {
                        this->next = 0;
                }
                char pitch;
                char velocity;
                Note *next;
};

bool higherPitch(Note *a, Note *b) {
	return (a->pitch > b->pitch);
}
bool lowerPitch(Note *a, Note *b) {
	return (a->pitch < b->pitch);
}
bool higherVelocity(Note *a, Note *b) {
	return (a->velocity > b->velocity);
}

class NoteBook {
	public:
		NoteBook(NB_Mode _mode = velocity) { 
			head = 0;
			setMode(_mode);
		};
		Note *noteOn(char _pitch, char _velocity);
		Note *noteOff(char _pitch);
		void setMode(NB_Mode mode);
		int allNotesOff();
	private:
		Note *head;
		bool (*comesBefore)(Note *a, Note *b);
};

void NoteBook::setMode(NB_Mode mode) {
	switch (mode) {
		case velocity:
			comesBefore = higherVelocity;
			break;
		case highest:
			comesBefore = higherPitch;
			break;
		case lowest:
			comesBefore = lowerPitch;
			break;
	}
}

Note *NoteBook::noteOn(char _pitch, char _velocity) {
	Note *newNote = new Note(_pitch, _velocity);

	if ((head == 0) || (comesBefore(newNote, head))) {
		newNote->next = head;
		head = newNote;
		return head;
	}

	Note *previous, *current = head;
	do {
		previous = current;
		current = current->next;
	} while ((current != 0) && !comesBefore(newNote, current));

	newNote->next = current;
	previous->next = newNote;

	return head;
}

Note *NoteBook::noteOff(char _pitch) {
	if (head == 0) {
		return head;
	}
	Note *current, *previous;
	while ((head != 0) && (head->pitch == _pitch)) {
		current = head;
		head = head->next;
		delete current;
	}
	current = head;
	while (current != 0) {

                previous = current;
                current = current->next;

	        if ((current != 0) && (current->pitch == _pitch)) {
                	previous->next = current->next;
        	        delete current;
			current = previous->next;
	        }
	}
	return head;
}

int NoteBook::allNotesOff() {
	Note *current = head, *previous;
	int count = 0;
	while (current != 0) {
		previous = current;
		current = current->next;
		delete previous;
		count++;
	}
	return count;
}
