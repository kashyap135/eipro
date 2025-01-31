const express = require('express');
const fs = require('fs');
const bodyParser = require('body-parser');
const path = require('path');
const cors = require('cors');
const app = express();
const PORT = 3000;
const crypto = require('crypto');
app.use(cors());
app.use(bodyParser.json()); // Middleware to parse JSON bodies

// File paths for user and group data
const usersFilePath = path.join(__dirname, 'users.json');
const groupsFilePath = path.join(__dirname, 'groups.json');
const profilesFilePath = path.join(__dirname, 'profiles.json');

// Helper function to read JSON data from a file
function readData(filePath) {
    try {
        const data = fs.readFileSync(filePath, 'utf8');
        return JSON.parse(data);
    } catch (err) {
        return []; // Return an empty array if the file doesn't exist or is empty
    }
}

// Helper function to write JSON data to a file
function writeData(filePath, data) {
    fs.writeFileSync(filePath, JSON.stringify(data, null, 2), 'utf8');
}

// ---- USER ROUTES ----

// 1. User Registration (POST /api/register)
app.post('/api/register', (req, res) => {
    const { name, email, mobile, password, Major } = req.body;

    const users = readData(usersFilePath);

    if (users.some(user => user.email === email)) {
        return res.status(400).json({ message: 'User already exists!' });
    }

    const newUser = { id: users.length + 1, name, email, mobile, password, Major };
    users.push(newUser);
    writeData(usersFilePath, users);

    res.status(201).json({ message: 'User registered successfully!', user: newUser });
});

// 2. User Login (POST /api/login)
app.post('/api/login', (req, res) => {
    const { email, password } = req.body;

    const users = readData(usersFilePath);
    const user = users.find(u => u.email === email && u.password === password);

    if (!user) {
        return res.status(401).json({ message: 'Invalid credentials' });
    }

    res.status(200).json({ message: 'Login successful', user });
});

// ---- PROFILE ROUTES ----

// 3. Get Profile (GET /api/profile/:userId)
app.get('/api/profile/:userId', (req, res) => {
    const userId = parseInt(req.params.userId, 10);

    const users = readData(usersFilePath);
    const user = users.find(u => u.id === userId);

    if (!user) {
        return res.status(404).json({ message: 'User not found' });
    }

    const profiles = readData(profilesFilePath);
    const profile = profiles.find(p => p.userId === userId);

    res.status(200).json({ user, profile: profile || {} });
});

// 4. Update Profile (PUT /api/profile/:userId)
app.put('/api/profile/:userId', (req, res) => {
    const userId = parseInt(req.params.userId, 10);
    const { name, mobile, Major } = req.body;

    const users = readData(usersFilePath);
    const userIndex = users.findIndex(u => u.id === userId);

    if (userIndex === -1) {
        return res.status(404).json({ message: 'User not found' });
    }

    // Update user details
    users[userIndex] = { ...users[userIndex], name, mobile, Major };
    writeData(usersFilePath, users);

    // Handle profile update
    const profiles = readData(profilesFilePath);
    const profileIndex = profiles.findIndex(p => p.userId === userId);

    if (profileIndex === -1) {
        const newProfile = { userId, major: Major };
        profiles.push(newProfile);
        writeData(profilesFilePath, profiles);
    } else {
        profiles[profileIndex] = { ...profiles[profileIndex], major: Major };
        writeData(profilesFilePath, profiles);
    }

    res.status(200).json({ message: 'Profile updated successfully' });
});

// ---- GROUP ROUTES ----

// 5. Create a Group (POST /api/groups)
app.post('/api/groups', (req, res) => {
    const { name, course, schedule, location, creatorId } = req.body;

    const users = readData(usersFilePath);
    const groups = readData(groupsFilePath);

    // Check if the user exists
    const creator = users.find(user => user.id === creatorId);
    if (!creator) {
        return res.status(404).json({ message: 'User not found' });
    }

    const newGroup = {
        id: groups.length + 1,
        name,
        course,
        schedule,
        location,
        creatorId,
        members: [creatorId], // Initially, the creator is the only member
        createdAt: new Date().toISOString(),
        inviteToken: crypto.randomBytes(16).toString('hex'), // Generate a unique invite token
    };
    groups.push(newGroup);
    writeData(groupsFilePath, groups);
    

    res.status(201).json({ message: 'Group created successfully!', group: newGroup });
});

// 6. Get All Groups (GET /api/groups)
app.get('/api/groups', (req, res) => {
    const groups = readData(groupsFilePath);
    res.status(200).json({ groups });
});

// 7. Get Group Details (GET /api/groups/:id)
app.get('/api/groups/:id', (req, res) => {
    const groupId = parseInt(req.params.id, 10);
    const userId = parseInt(req.query.userId, 10); // Get userId from query parameter

    const groups = readData(groupsFilePath);
    const group = groups.find(g => g.id === groupId);

    if (!group) {
        return res.status(404).json({ message: 'Group not found' });
    }

    // Add user permissions to the response
    const userPermissions = {
        canDelete: group.creatorId === userId, // Only creator can delete
        canLeave: group.members.includes(userId) && group.creatorId !== userId, // Members can leave, but creator can't
        isMember: group.members.includes(userId)
    };

    res.status(200).json({
        ...group,
        userPermissions
    });
});

// 8. Join a Group (POST /api/groups/join)
app.post('/api/groups/join', (req, res) => {
    const { groupId, userId } = req.body;

    const users = readData(usersFilePath);
    const groups = readData(groupsFilePath);

    const user = users.find(u => u.id === userId);
    if (!user) {
        return res.status(404).json({ message: 'User not found' });
    }

    const groupIndex = groups.findIndex(g => g.id === groupId);
    if (groupIndex === -1) {
        return res.status(404).json({ message: 'Group not found' });
    }

    // Add user to the group if they are not already a member
    if (!groups[groupIndex].members.includes(userId)) {
        groups[groupIndex].members.push(userId);
        writeData(groupsFilePath, groups);
        res.status(200).json({ 
            message: 'Successfully joined the group',
            group: groups[groupIndex]
        });
    } else {
        res.status(400).json({ message: 'User is already a member of this group' });
    }
});

// 9. Leave a Group (POST /api/groups/leave)
app.post('/api/groups/leave', (req, res) => {
    const { groupId, userId } = req.body;

    const groups = readData(groupsFilePath);
    const groupIndex = groups.findIndex(g => g.id === groupId);

    if (groupIndex === -1) {
        return res.status(404).json({ message: 'Group not found' });
    }

    const group = groups[groupIndex];

    // Check if user is the creator
    if (group.creatorId === userId) {
        return res.status(400).json({ 
            message: 'Group creator cannot leave the group. Please delete the group instead.'
        });
    }

    // Check if user is a member
    if (!group.members.includes(userId)) {
        return res.status(400).json({ message: 'User is not a member of this group' });
    }

    // Remove user from the group
    group.members = group.members.filter(memberId => memberId !== userId);
    groups[groupIndex] = group;
    writeData(groupsFilePath, groups);

    res.status(200).json({ 
        message: 'Successfully left the group',
        group: group
    });
});

// 10. Delete a Group (DELETE /api/groups/:id)
app.delete('/api/groups/:id', (req, res) => {
    const groupId = parseInt(req.params.id, 10);
    const userId = parseInt(req.body.userId, 10);

    const groups = readData(groupsFilePath);
    const groupIndex = groups.findIndex(g => g.id === groupId);

    if (groupIndex === -1) {
        return res.status(404).json({ message: 'Group not found' });
    }

    // Check if the user is the creator
    if (groups[groupIndex].creatorId !== userId) {
        return res.status(403).json({ message: 'Only the group creator can delete the group' });
    }

    // Remove the group
    groups.splice(groupIndex, 1);
    writeData(groupsFilePath, groups);

    res.status(200).json({ message: 'Group deleted successfully' });
});
// Route to get the invite link for a group
app.get('/api/groups/:id/invite', (req, res) => {
    const groupId = parseInt(req.params.id, 10);

    const groups = readData(groupsFilePath);
    const group = groups.find(g => g.id === groupId);

    if (!group) {
        return res.status(404).json({ message: 'Group not found' });
    }

    const inviteLink = `http://localhost:${PORT}/api/groups/invite/${group.inviteToken}`;
    res.status(200).json({ message: 'Invite link generated successfully', inviteLink });
});

// Route to join a group using an invite token
app.post('/api/groups/invite/:token', (req, res) => {
    const { token } = req.params;
    const { userId } = req.body;

    const users = readData(usersFilePath);
    const groups = readData(groupsFilePath);

    // Check if the user exists
    const user = users.find(u => u.id === userId);
    if (!user) {
        return res.status(404).json({ message: 'User not found' });
    }

    // Find the group with the given invite token
    const group = groups.find(g => g.inviteToken === token);

    if (!group) {
        return res.status(404).json({ message: 'Invalid or expired invite link' });
    }

    // Add the user to the group if they are not already a member
    if (!group.members.includes(userId)) {
        group.members.push(userId);
        writeData(groupsFilePath, groups);
        res.status(200).json({ 
            message: 'Successfully joined the group',
            group
        });
    } else {
        res.status(400).json({ message: 'User is already a member of this group' });
    }
});

// Start the server
app.listen(PORT, () => {
    console.log(`Server is running on http://localhost:${PORT}`);
});